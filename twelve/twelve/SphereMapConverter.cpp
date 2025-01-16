#include "SphereMapConverter.h"

#include <CommonStates.h>
#include <DirectXHelpers.h>
#include <SimpleMath.h>
#include "Logger.h"

using namespace DirectX::SimpleMath;

namespace
{
#include "../Compiled/SphereToCubeVS.inc"
#include "../Compiled/SphereToCubePS.inc"

	struct alignas(256) CbTransform
	{
		Matrix World;
		Matrix View;
		Matrix Proj;
	};
}

SphereMapConverter::SphereMapConverter()
	: m_pPoolRes(nullptr)
	, m_pPoolRTV(nullptr)
{
}

SphereMapConverter::~SphereMapConverter()
{
	Term();
}

bool SphereMapConverter::Init(ID3D12Device* pDevice, DescriptorPool* pPoolRTV, DescriptorPool* pPoolRes, const D3D12_RESOURCE_DESC& sphereMapDesc, int mapSize)
{
	if (pDevice == nullptr || pPoolRTV == nullptr || pPoolRes == nullptr)
	{
		ELOG("Error : Invalid Argument.");
		return false;
	}

	m_pPoolRTV = pPoolRTV;
	m_pPoolRTV->AddRef();

	m_pPoolRes = pPoolRes;
	m_pPoolRes->AddRef();

	// ルートシグネチャの生成
	{
		RootSignature::Desc desc;
		desc.Begin(2)
			.SetCBV(ShaderStage::VS, 0, 0)
			.SetSRV(ShaderStage::PS, 1, 0)
			.AddStaticSmp(ShaderStage::PS, 0, SamplerState::AnisotropicWrap)
			.AllowIL()
			.End();

		if (!m_RootSignature.Init(pDevice, desc.GetDesc()))
		{
			ELOG("Error : RootSignature::Init() Failed.");
			return false;
		}
	}

	// パイプラインステートの生成
	{
		D3D12_INPUT_ELEMENT_DESC elements[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_RootSignature.GetPtr();
		desc.VS = { SphereToCubeVS, sizeof(SphereToCubeVS) };
		desc.PS = { SphereToCubePS, sizeof(SphereToCubePS) };
		desc.BlendState = DirectX::CommonStates::Opaque;
		desc.SampleMask = UINT_MAX;
		desc.RasterizerState = DirectX::CommonStates::CullNone;
		desc.DepthStencilState = DirectX::CommonStates::DepthNone;
		desc.InputLayout = { elements, _countof(elements) };
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		auto hr = pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pPSO.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
			return false;
		}
	}

	// テクスチャの生成.
	{
		// 暫定サイズを求める.
		auto tempSize = (mapSize == -1) ? sphereMapDesc.Width / 4 : uint32_t(mapSize);

		uint32_t currSize = 1;
		uint32_t currMipLevels = 1;

		uint32_t prevSize = 0;

		uint32_t size = 0;
		uint32_t mipLevels = 0;

		// ループしながら一番近い2のべき乗を探す
		for (;;)
		{
			// 暫定サイズが含まれるかどうか判定
			if (prevSize < tempSize && tempSize <= currSize)
			{
				size = currSize;
				mipLevels = currMipLevels;
				break;
			}

			// 前のサイズを更新
			prevSize = currSize;

			// テクスチャサイズを２倍
			currSize <<= 1;

			// ミップレベルをカウントアップ
			currMipLevels++;
		}

		m_MipCount = mipLevels;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = size;
		desc.Height = UINT(size);
		desc.DepthOrArraySize = 6;
		desc.MipLevels = m_MipCount;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		m_pCubeTex = std::make_unique<Texture>();
		if (!m_pCubeTex->Init(pDevice, m_pPoolRes, &desc, false, true))
		{
			ELOG("Error : Texture::Init() Failed.");
			return false;
		}
	}

	// レンダーターゲットの生成
	// TODO: ColorTagetで初期化
	{
		m_pCubeRTVs.resize(m_MipCount * 6);

		auto idx = 0;
		for (auto i = 0; i < 6; ++i)
		{
			for (auto m = 0u; m < m_MipCount; ++m)
			{
				auto pHandle = m_pPoolRTV->AllocHandle();
				if (pHandle == nullptr)
				{
					ELOG("Error : DescriptorPool::AllocHandle() Failed.");
					return false;
				}

				m_pCubeRTVs[idx] = pHandle;

				D3D12_RENDER_TARGET_VIEW_DESC desc = {};
				desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				desc.Texture2DArray.ArraySize = 1;
				desc.Texture2DArray.FirstArraySlice = i;
				desc.Texture2DArray.MipSlice = m;
				desc.Texture2DArray.PlaneSlice = 0;

				pDevice->CreateRenderTargetView(m_pCubeTex->GetComPtr().Get(), &desc, pHandle->HandleCPU);

				idx++;
			}
		}
	}

	// 変換用バッファの生成
	{
		auto pos = Vector3(0.0f, 0.0f, 0.0f);
		Vector3 target[6] = {
			Vector3(1.0f,  0.0f,  0.0f),
			Vector3(-1.0f,  0.0f,  0.0f),

			Vector3(0.0f,  1.0f,  0.0f),
			Vector3(0.0f, -1.0f,  0.0f),

			Vector3(0.0f,  0.0f, -1.0f),
			Vector3(0.0f,  0.0f,  1.0f),
		};
		Vector3 upward[6] = {
			Vector3(0.0f, 1.0f,  0.0f),
			Vector3(0.0f, 1.0f,  0.0f),

			Vector3(0.0f, 0.0f,  1.0f),
			Vector3(0.0f, 0.0f, -1.0f),

			Vector3(0.0f, 1.0f,  0.0f),
			Vector3(0.0f, 1.0f,  0.0f),
		};

		for (auto i = 0u; i < 6; ++i)
		{
			if (!m_TransformCB[i].Init(pDevice, pPoolRes, sizeof(CbTransform)))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");
				return false;
			}

			auto ptr = m_TransformCB[i].GetPtr<CbTransform>();
			ptr->World = Matrix::Identity;
			ptr->View = Matrix::CreateLookAt(pos, target[i], upward[i]);
			ptr->Proj = Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PIDIV2, 1.0f, 0.1f, 1000.0f);
		}
	}

	// 頂点バッファとインデックスバッファの生成
	{
		struct Vertex
		{
			Vector3 Position;
			Vector2 TexCoord;
		};

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		// 分割数
		auto tessellation = 20u;

		uint32_t verticalSegments = tessellation * 2;
		uint32_t horizontalSegments = tessellation * 2;

		// 球の半径
		float radius = 10.0f;

		// 頂点座標を求める
		for (size_t i = 0; i <= verticalSegments; i++)
		{
			float v = 1.0f - (float)i / (verticalSegments);

			float latitude = (i * DirectX::XM_PI / verticalSegments) - DirectX::XM_PIDIV2;
			float dy, dxz;

			dy = sinf(latitude);
			dxz = cosf(latitude);

			for (size_t j = 0; j <= horizontalSegments; j++)
			{
				float u = (float)j / horizontalSegments;

				float longitude = j * DirectX::XM_2PI / horizontalSegments;
				float dx, dz;

				dx = cosf(longitude) * dxz;
				dz = sinf(longitude) * dxz;

				auto normal = Vector3(dx, dy, dz);
				auto uv = Vector2(u, v);

				auto l = normal.x * normal.x + normal.z * normal.z;

				Vertex vertex;
				vertex.Position = normal * radius;
				vertex.TexCoord = uv;

				vertices.push_back(vertex);
			}
		}

		uint32_t stride = horizontalSegments + 1;

		// 頂点インデックスを求める
		for (auto i = 0u; i < verticalSegments; i++)
		{
			for (auto j = 0u; j <= horizontalSegments; j++)
			{
				auto nextI = i + 1;
				auto nextJ = (j + 1) % stride;

				indices.push_back(i * stride + j);
				indices.push_back(nextI * stride + j);
				indices.push_back(i * stride + nextJ);

				indices.push_back(i * stride + nextJ);
				indices.push_back(nextI * stride + j);
				indices.push_back(nextI * stride + nextJ);
			}
		}

		if (!m_VB.Init<Vertex>(pDevice, vertices.size(), vertices.data()))
		{
			ELOG("Error : VertexBuffer::Init() Failed.");
			return false;
		}

		if (!m_IB.Init(pDevice, indices.size(), indices.data()))
		{
			ELOG("Error : IndexBuffer::Init() Failed.");
			return false;
		}
	}

	return true;
}

void SphereMapConverter::Term()
{
	m_VB.Term();
	m_IB.Term();
	m_RootSignature.Term();
	m_pPSO.Reset();

	for (auto i = 0; i < 6; ++i)
	{
		m_TransformCB[i].Term();
	}

	m_pCubeTex->Term();

	if (m_pPoolRTV != nullptr)
	{
		for (size_t i = 0; i < m_pCubeRTVs.size(); ++i)
		{
			if (m_pCubeRTVs[i] != nullptr)
			{
				m_pPoolRTV->FreeHandle(m_pCubeRTVs[i]);
				m_pCubeRTVs[i] = nullptr;
			}
		}

		m_pCubeRTVs.clear();
		m_pCubeRTVs.shrink_to_fit();

		m_pPoolRTV->Release();
		m_pPoolRTV = nullptr;
	}
}

void SphereMapConverter::DrawToCube(ID3D12GraphicsCommandList* pCmd, D3D12_GPU_DESCRIPTOR_HANDLE sphereMapHandle)
{
	DirectX::TransitionResource(
		pCmd,
		m_pCubeTex->GetComPtr().Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto desc = m_pCubeTex->GetDesc();

	auto idx = 0;
	auto handleSRV = sphereMapHandle;
	for (auto i = 0; i < 6; ++i)
	{
		auto w = uint32_t(desc.Width);
		auto h = desc.Height;

		for (auto m = 0u; m < m_MipCount; ++m)
		{
			auto handleRTV = m_pCubeRTVs[idx]->HandleCPU;

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

			pCmd->OMSetRenderTargets(1, &handleRTV, FALSE, nullptr);

			pCmd->SetGraphicsRootSignature(m_RootSignature.GetPtr());
			pCmd->SetGraphicsRootDescriptorTable(0, m_TransformCB[i].GetHandleGPU());
			pCmd->SetGraphicsRootDescriptorTable(1, handleSRV);
			pCmd->SetPipelineState(m_pPSO.Get());
			pCmd->RSSetViewports(1, &viewport);
			pCmd->RSSetScissorRects(1, &scissor);

			DrawSphere(pCmd);

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
		m_pCubeTex->GetComPtr().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

D3D12_RESOURCE_DESC SphereMapConverter::GetCubeMapDesc() const
{
	return m_pCubeTex->GetDesc();
}

D3D12_CPU_DESCRIPTOR_HANDLE SphereMapConverter::GetCubeMapHandleCPU() const
{
	return m_pCubeTex->GetHandleCPU();
}

D3D12_GPU_DESCRIPTOR_HANDLE SphereMapConverter::GetCubeMapHandleGPU() const
{
	return m_pCubeTex->GetHandleGPU();
}

void SphereMapConverter::DrawSphere(ID3D12GraphicsCommandList* pCmd)
{
	auto vbv = m_VB.GetView();
	auto ibv = m_IB.GetView();

	pCmd->IASetVertexBuffers(0, 1, &vbv);
	pCmd->IASetIndexBuffer(&ibv);
	pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pCmd->DrawIndexedInstanced(UINT(m_IB.GetCount()), 1, 0, 0, 0);
}
