#include "IBLBaker.h"

#include <SimpleMath.h>
#include <CommonStates.h>
#include <DirectXHelpers.h>
#include <pix_win.h>
#include "Logger.h"

using namespace DirectX::SimpleMath;

namespace
{
#include "../Compiled/QuadVS.inc"
#include "../Compiled/IntegrateDFG_PS.inc"
#include "../Compiled/IntegrateDiffuseLD_PS.inc"
#include "../Compiled/IntegrateSpecularLD_PS.inc"

	struct alignas(256) CbBake
	{
		int FaceIndex;
		float Roughness;
		float Width;
		float MipCount;
	};
}

IBLBaker::IBLBaker()
	: m_pPoolRes(nullptr)
	, m_pPoolRTV(nullptr)
	, m_pHandleSRV_DFG(nullptr)
	, m_pHandleSRV_DiffuseLD(nullptr)
	, m_pHandleSRV_SpecularLD(nullptr)
{
	for (auto i = 0; i < MipCount * 6; i++)
	{
		m_pHandleRTV_SpecularLD[i] = nullptr;
	}

	for (auto i = 0; i < 6; i++)
	{
		m_pHandleRTV_DiffuseLD[i] = nullptr;
	}
}

IBLBaker::~IBLBaker()
{
	Term();
}

bool IBLBaker::Init(ID3D12Device* pDevice, DescriptorPool* pPoolRes, DescriptorPool* pPoolRTV)
{
	// 頂点バッファの生成
	{
		struct Vertex
		{
			Vector2 Position;
			Vector2 TexCoord;
		};

		Vertex vertices[] = {
			{ Vector2(-1.0f,  1.0f), Vector2(0.0f,  1.0f) },
			{ Vector2(3.0f,  1.0f), Vector2(2.0f,  1.0f) },
			{ Vector2(-1.0f, -3.0f), Vector2(0.0f, -1.0f) },
		};

		if (!m_QuadVB.Init<Vertex>(pDevice, 3, vertices))
		{
			ELOG("Error : VertexBuffer::Init() Failed.");
			return false;
		}
	}

	m_pPoolRes = pPoolRes;
	m_pPoolRes->AddRef();

	m_pPoolRTV = pPoolRTV;
	m_pPoolRTV->AddRef();

	// DFG項積分用ルートシグネチャの生成
	{
		RootSignature::Desc desc;
		desc.Begin(1)
			.SetCBV(ShaderStage::PS, 0, 0)
			.AddStaticSmp(ShaderStage::PS, 0, SamplerState::AnisotropicWrap)
			.AllowIL()
			.End();

		if (!m_DFG_RootSignature.Init(pDevice, desc.GetDesc()))
		{
			ELOG("Error : RootSignature::Init() Failed.");
			return false;
		}
	}

	// LD項積分用ルートシグネチャの生成
	{
		RootSignature::Desc desc;
		desc.Begin(2)
			.SetCBV(ShaderStage::PS, 0, 0)
			.SetSRV(ShaderStage::PS, 1, 0)
			.AddStaticSmp(ShaderStage::PS, 0, SamplerState::AnisotropicWrap)
			.AllowIL()
			.End();

		if (!m_LD_RootSignature.Init(pDevice, desc.GetDesc()))
		{
			ELOG("Error : RootSignature::Init() Failed.");
			return false;
		}
	}

	D3D12_INPUT_ELEMENT_DESC elements[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	auto format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	// DFG積分用パイプラインステートの生成
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.InputLayout = { elements, _countof(elements) };
		desc.pRootSignature = m_DFG_RootSignature.GetPtr();
		desc.VS = { QuadVS, sizeof(QuadVS) };
		desc.PS = { IntegrateDFG_PS, sizeof(IntegrateDFG_PS) };
		desc.RasterizerState = DirectX::CommonStates::CullNone;
		desc.BlendState = DirectX::CommonStates::Opaque;
		desc.DepthStencilState = DirectX::CommonStates::DepthNone;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = DXGI_FORMAT_R32G32_FLOAT;
		desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		auto hr = pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pDFG_PSO.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
			return false;
		}
	}

	// Diffuse LD積分用パイプラインステートの生成
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.InputLayout = { elements, _countof(elements) };
		desc.pRootSignature = m_LD_RootSignature.GetPtr();
		desc.VS = { QuadVS, sizeof(QuadVS) };
		desc.PS = { IntegrateDiffuseLD_PS, sizeof(IntegrateDiffuseLD_PS) };
		desc.RasterizerState = DirectX::CommonStates::CullNone;
		desc.BlendState = DirectX::CommonStates::Opaque;
		desc.DepthStencilState = DirectX::CommonStates::DepthNone;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = format;
		desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		auto hr = pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pDiffuseLD_PSO.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
			return false;
		}
	}

	// Specular LD積分用パイプラインステートの生成
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.InputLayout = { elements, _countof(elements) };
		desc.pRootSignature = m_LD_RootSignature.GetPtr();
		desc.VS = { QuadVS, sizeof(QuadVS) };
		desc.PS = { IntegrateSpecularLD_PS, sizeof(IntegrateSpecularLD_PS) };
		desc.RasterizerState = DirectX::CommonStates::CullNone;
		desc.BlendState = DirectX::CommonStates::Opaque;
		desc.DepthStencilState = DirectX::CommonStates::DepthNone;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = format;
		desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		auto hr = pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pSpecularLD_PSO.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
			return false;
		}
	}

	// DFG積分用レンダーターゲットの生成.
	{
		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Width = DFGTextureSize;
		texDesc.Height = DFGTextureSize;
		texDesc.DepthOrArraySize = 1;
		texDesc.MipLevels = 1;
		texDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_HEAP_PROPERTIES props = {};
		props.Type = D3D12_HEAP_TYPE_DEFAULT;
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_R32G32_FLOAT;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;

		auto hr = pDevice->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(m_pTexDFG.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateCommittedResource() Failed.");
			return false;
		}

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		m_pHandleRTV_DFG = m_pPoolRTV->AllocHandle();
		if (m_pHandleRTV_DFG == nullptr)
		{
			ELOG("Error : Descriptor Handle Allocate Failed.");
			return false;
		}

		pDevice->CreateRenderTargetView(m_pTexDFG.Get(), &rtvDesc, m_pHandleRTV_DFG->HandleCPU);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		m_pHandleSRV_DFG = m_pPoolRes->AllocHandle();
		if (m_pHandleSRV_DFG == nullptr)
		{
			ELOG("Error : Descriptor Handle Allocate Failed.");
			return false;
		}

		pDevice->CreateShaderResourceView(m_pTexDFG.Get(), &srvDesc, m_pHandleSRV_DFG->HandleCPU);
	}

	// Diffuse LD積分用レンダーターゲットの生成.
	{
		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Width = LDTextureSize;
		texDesc.Height = LDTextureSize;
		texDesc.DepthOrArraySize = 6;
		texDesc.MipLevels = 1;
		texDesc.Format = format;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_HEAP_PROPERTIES props = {};
		props.Type = D3D12_HEAP_TYPE_DEFAULT;
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = format;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;

		auto hr = pDevice->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(m_pTexDiffuseLD.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateCommittedResource() Failed.");
			return false;
		}

		for (auto i = 0; i < 6; ++i)
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = format;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.ArraySize = 1;
			rtvDesc.Texture2DArray.FirstArraySlice = i;
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.PlaneSlice = 0;

			m_pHandleRTV_DiffuseLD[i] = m_pPoolRTV->AllocHandle();
			if (m_pHandleRTV_DiffuseLD[i] == nullptr)
			{
				ELOG("Error : Descriptor Handle Allocate Failed.");
				return false;
			}

			pDevice->CreateRenderTargetView(m_pTexDiffuseLD.Get(), &rtvDesc, m_pHandleRTV_DiffuseLD[i]->HandleCPU);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.TextureCube.MipLevels = 1;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0;

		m_pHandleSRV_DiffuseLD = m_pPoolRes->AllocHandle();
		if (m_pHandleSRV_DiffuseLD == nullptr)
		{
			ELOG("Error : Descriptor Handle Allocate Failed.");
			return false;
		}

		pDevice->CreateShaderResourceView(m_pTexDiffuseLD.Get(), &srvDesc, m_pHandleSRV_DiffuseLD->HandleCPU);
	}

	// Specular LD積分用レンダーターゲットの生成.
	{
		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Width = LDTextureSize;
		texDesc.Height = LDTextureSize;
		texDesc.DepthOrArraySize = 6;
		texDesc.MipLevels = MipCount;
		texDesc.Format = format;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_HEAP_PROPERTIES props = {};
		props.Type = D3D12_HEAP_TYPE_DEFAULT;
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = format;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;

		auto hr = pDevice->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(m_pTexSpecularLD.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateCommittedResource() Failed.");
			return false;
		}

		auto idx = 0;
		for (auto i = 0; i < 6; ++i)
		{
			for (auto m = 0; m < MipCount; ++m)
			{
				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = format;
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				rtvDesc.Texture2DArray.ArraySize = 1;
				rtvDesc.Texture2DArray.FirstArraySlice = i;
				rtvDesc.Texture2DArray.MipSlice = m;
				rtvDesc.Texture2DArray.PlaneSlice = 0;

				m_pHandleRTV_SpecularLD[idx] = m_pPoolRTV->AllocHandle();
				if (m_pHandleRTV_SpecularLD[idx] == nullptr)
				{
					ELOG("Error : Descriptor Handle Allocate Failed.");
					return false;
				}

				pDevice->CreateRenderTargetView(m_pTexSpecularLD.Get(), &rtvDesc, m_pHandleRTV_SpecularLD[idx]->HandleCPU);
				idx++;
			}
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.TextureCube.MipLevels = MipCount;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0;

		m_pHandleSRV_SpecularLD = m_pPoolRes->AllocHandle();
		if (m_pHandleSRV_SpecularLD == nullptr)
		{
			ELOG("Error : Descriptor Handle Allocate Failed.");
			return false;
		}

		pDevice->CreateShaderResourceView(m_pTexSpecularLD.Get(), &srvDesc, m_pHandleSRV_SpecularLD->HandleCPU);
	}

	// 定数バッファの生成
	{
		const auto RoughnessStep = 1.0f / float(MipCount - 1);

		auto idx = 0;
		for (auto i = 0; i < 6; ++i)
		{
			auto roughness = 0.0f;

			for (auto m = 0; m < MipCount; ++m)
			{
				if (!m_BakeCB[idx].Init(pDevice, pPoolRes, sizeof(CbBake)))
				{
					ELOG("Error : ConstantBuffer::Init() Failed.");
					return false;
				}

				auto ptr = m_BakeCB[idx].GetPtr<CbBake>();
				ptr->FaceIndex = i;
				ptr->MipCount = MipCount - 1;
				ptr->Roughness = roughness;
				ptr->Width = LDTextureSize;

				idx++;

				roughness += RoughnessStep;
			}
		}
	}

	return true;
}

void IBLBaker::Term()
{
	for (auto i = 0; i < 6; ++i)
	{
		if (m_pPoolRTV != nullptr)
		{
			if (m_pHandleRTV_DiffuseLD[i] != nullptr)
			{
				m_pPoolRTV->FreeHandle(m_pHandleRTV_DiffuseLD[i]);
			}
		}
	}

	for (auto i = 0; i < MipCount * 6; ++i)
	{
		m_BakeCB[i].Term();

		if (m_pPoolRTV != nullptr)
		{
			if (m_pHandleRTV_SpecularLD[i] != nullptr)
			{
				m_pPoolRTV->FreeHandle(m_pHandleRTV_SpecularLD[i]);
			}
		}
	}

	m_QuadVB.Term();

	if (m_pPoolRTV != nullptr)
	{
		if (m_pHandleRTV_DFG != nullptr)
		{
			m_pPoolRTV->FreeHandle(m_pHandleRTV_DFG);
		}
	}

	if (m_pPoolRes != nullptr)
	{
		if (m_pHandleSRV_DFG != nullptr)
		{
			m_pPoolRes->FreeHandle(m_pHandleSRV_DFG);
		}

		if (m_pHandleSRV_DiffuseLD != nullptr)
		{
			m_pPoolRes->FreeHandle(m_pHandleSRV_DiffuseLD);
		}

		if (m_pHandleRTV_SpecularLD != nullptr)
		{
			m_pPoolRes->FreeHandle(m_pHandleSRV_SpecularLD);
		}
	}

	if (m_pPoolRes != nullptr)
	{
		m_pPoolRes->Release();
		m_pPoolRes = nullptr;
	}

	if (m_pPoolRTV != nullptr)
	{
		m_pPoolRTV->Release();
		m_pPoolRTV = nullptr;
	}

	m_pTexDFG.Reset();
	m_pTexDiffuseLD.Reset();
	m_pTexSpecularLD.Reset();
	m_pDFG_PSO.Reset();
	m_pDiffuseLD_PSO.Reset();
	m_pSpecularLD_PSO.Reset();
	m_DFG_RootSignature.Term();
	m_LD_RootSignature.Term();
}

void IBLBaker::IntegrateDFG(ID3D12GraphicsCommandList* pCmd)
{
	auto pVBV = m_QuadVB.GetView();
	auto pRTV = m_pHandleRTV_DFG->HandleCPU;

	DirectX::TransitionResource(
		pCmd,
		m_pTexDFG.Get(),
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = float(DFGTextureSize);
	viewport.Height = float(DFGTextureSize);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissor = {};
	scissor.left = 0;
	scissor.right = DFGTextureSize;
	scissor.top = 0;
	scissor.bottom = DFGTextureSize;

	pCmd->OMSetRenderTargets(1, &pRTV, FALSE, nullptr);
	pCmd->SetGraphicsRootSignature(m_DFG_RootSignature.GetPtr());
	pCmd->SetPipelineState(m_pDFG_PSO.Get());
	pCmd->RSSetViewports(1, &viewport);
	pCmd->RSSetScissorRects(1, &scissor);

	pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCmd->IASetVertexBuffers(0, 1, &pVBV);
	pCmd->IASetIndexBuffer(nullptr);
	pCmd->DrawInstanced(3, 1, 0, 0);

	DirectX::TransitionResource(
		pCmd,
		m_pTexDFG.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void IBLBaker::IntegrateLD(ID3D12GraphicsCommandList* pCmd, uint32_t mapSize, uint32_t mipCount, D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
	auto idx = 0;

	// 出力テクスチャのミップマップ数で割ってステップ数を求める
	// ※ 入力テクスチャサイズによって mipCount != MipCount となることに注意
	const auto RoughnessStep = 1.0f / (MipCount - 1);

	for (auto i = 0; i < 6; ++i)
	{
		auto roughness = 0.0f;

		// 出力テクスチャのミップマップ数分ループする
		for (auto m = 0; m < MipCount; ++m)
		{
			auto ptr = m_BakeCB[idx].GetPtr<CbBake>();
			ptr->FaceIndex = i;
			ptr->MipCount = float(mipCount - 1);
			ptr->Width = float(mapSize);
			ptr->Roughness = roughness * roughness;

			idx++;
			roughness += RoughnessStep;
		}
	}

	// ルートシグネチャを設定
	pCmd->SetGraphicsRootSignature(m_LD_RootSignature.GetPtr());

	IntegrateDiffuseLD(pCmd, handle);
	IntegrateSpecularLD(pCmd, handle);
}

D3D12_CPU_DESCRIPTOR_HANDLE IBLBaker::GetHandleCPU_DFG() const
{
	return m_pHandleSRV_DFG->HandleCPU;
}

D3D12_CPU_DESCRIPTOR_HANDLE IBLBaker::GetHandleCPU_DiffuseLD() const
{
	return m_pHandleSRV_DiffuseLD->HandleCPU;
}

D3D12_CPU_DESCRIPTOR_HANDLE IBLBaker::GetHandleCPU_SpecularLD() const
{
	return m_pHandleSRV_SpecularLD->HandleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE IBLBaker::GetHandleGPU_DFG() const
{
	return m_pHandleSRV_DFG->HandleGPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE IBLBaker::GetHandleGPU_DiffuseLD() const
{
	return m_pHandleSRV_DiffuseLD->HandleGPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE IBLBaker::GetHandleGPU_SpecularLD() const
{
	return m_pHandleSRV_SpecularLD->HandleGPU;
}

void IBLBaker::IntegrateDiffuseLD(ID3D12GraphicsCommandList* pCmd, D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
	auto pVBV = m_QuadVB.GetView();

	DirectX::TransitionResource(
		pCmd,
		m_pTexDiffuseLD.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = LDTextureSize;
	viewport.Height = LDTextureSize;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissor = {};
	scissor.left = 0;
	scissor.right = LDTextureSize;
	scissor.top = 0;
	scissor.bottom = LDTextureSize;

	for (auto i = 0; i < 6; ++i)
	{
		PIXBeginEvent(pCmd, 0, "IntegrateDiffuseLD%d", i);
		auto pRTV = m_pHandleRTV_DiffuseLD[i]->HandleCPU;
		pCmd->OMSetRenderTargets(1, &pRTV, FALSE, nullptr);
		pCmd->RSSetViewports(1, &viewport);
		pCmd->RSSetScissorRects(1, &scissor);
		pCmd->SetPipelineState(m_pDiffuseLD_PSO.Get());
		pCmd->SetGraphicsRootDescriptorTable(0, m_BakeCB[i * MipCount].GetHandleGPU());
		pCmd->SetGraphicsRootDescriptorTable(1, handle);

		pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pCmd->IASetVertexBuffers(0, 1, &pVBV);
		pCmd->IASetIndexBuffer(nullptr);
		pCmd->DrawInstanced(3, 1, 0, 0);
		PIXEndEvent(pCmd);
	}

	DirectX::TransitionResource(
		pCmd,
		m_pTexDiffuseLD.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void IBLBaker::IntegrateSpecularLD(ID3D12GraphicsCommandList* pCmd, D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
	auto pVBV = m_QuadVB.GetView();

	DirectX::TransitionResource(
		pCmd,
		m_pTexSpecularLD.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto idx = 0;
	for (auto i = 0; i < 6; ++i)
	{
		auto w = LDTextureSize;
		auto h = LDTextureSize;

		for (auto m = 0; m < MipCount; ++m)
		{
			D3D12_VIEWPORT viewport = {};
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			viewport.Width = float(w);
			viewport.Height = float(h);
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			D3D12_RECT scissor = {};
			scissor.left = 0;
			scissor.right = w;
			scissor.top = 0;
			scissor.bottom = h;

			auto pRTV = m_pHandleRTV_SpecularLD[idx]->HandleCPU;
			pCmd->OMSetRenderTargets(1, &pRTV, FALSE, nullptr);
			pCmd->RSSetViewports(1, &viewport);
			pCmd->RSSetScissorRects(1, &scissor);
			pCmd->SetPipelineState(m_pSpecularLD_PSO.Get());
			pCmd->SetGraphicsRootDescriptorTable(0, m_BakeCB[idx].GetHandleGPU());
			pCmd->SetGraphicsRootDescriptorTable(1, handle);

			pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			pCmd->IASetVertexBuffers(0, 1, &pVBV);
			pCmd->IASetIndexBuffer(nullptr);
			pCmd->DrawInstanced(3, 1, 0, 0);

			w >>= 1;
			h >>= 1;

			if (w < 1)
			{
				w = 1;
			}

			if (h < 1)
			{
				h = 1;
			}

			idx++;
		}
	}

	DirectX::TransitionResource(
		pCmd,
		m_pTexSpecularLD.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
