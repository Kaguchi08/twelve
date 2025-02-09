#include "SkyBox.h"

#include <CommonStates.h>
#include "Logger.h"

using namespace DirectX::SimpleMath;

namespace
{
#include "../Compiled/SkyBoxVS.inc"
#include "../Compiled/SkyBoxPS.inc"

	struct alignas(256) CbSkyBox
	{
		Matrix World;
		Matrix View;
		Matrix Proj;
	};
}

SkyBox::SkyBox()
	: m_pPoolRes(nullptr)
	, m_Index(0)
{
}

SkyBox::~SkyBox()
{
	Term();
}

bool SkyBox::Init(ID3D12Device* pDevice, DescriptorPool* pPoolRes, DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat)
{
	if (pDevice == nullptr || pPoolRes == nullptr)
	{
		ELOG("Error : Invalid Argument.");
		return false;
	}

	m_pPoolRes = pPoolRes;
	m_pPoolRes->AddRef();

	// ルートシグネチャの生成
	{
		RootSignature::Desc desc;
		desc.Begin(2)
			.SetCBV(ShaderStage::VS, 0, 0)
			.SetSRV(ShaderStage::PS, 1, 0)
			.AddStaticSmp(ShaderStage::PS, 0, SamplerState::LinearWrap)
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
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_RootSignature.GetPtr();
		desc.VS = { SkyBoxVS, sizeof(SkyBoxVS) };
		desc.PS = { SkyBoxPS, sizeof(SkyBoxPS) };
		desc.BlendState = DirectX::CommonStates::Opaque;
		desc.SampleMask = UINT_MAX;
		desc.RasterizerState = DirectX::CommonStates::CullNone;
		desc.DepthStencilState = DirectX::CommonStates::DepthRead;
		desc.InputLayout = { elements, 1 };
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = colorFormat;
		desc.DSVFormat = depthFormat;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		auto hr = pDevice->CreateGraphicsPipelineState(
			&desc, IID_PPV_ARGS(m_pPSO.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
			return false;
		}
	}

	// 頂点バッファの生成
	{
		Vector3 vertices[] = {
			Vector3(-1.0f,  1.0f, -1.0f),
			Vector3(-1.0f, -1.0f, -1.0f),
			Vector3(1.0f, -1.0f, -1.0f),

			Vector3(-1.0f,  1.0f, -1.0f),
			Vector3(1.0f, -1.0f, -1.0f),
			Vector3(1.0f,  1.0f, -1.0f),

			Vector3(1.0f,  1.0f, -1.0f),
			Vector3(1.0f, -1.0f, -1.0f),
			Vector3(1.0f, -1.0f,  1.0f),

			Vector3(1.0f,  1.0f, -1.0f),
			Vector3(1.0f, -1.0f,  1.0f),
			Vector3(1.0f,  1.0f,  1.0f),

			Vector3(1.0f,  1.0f, 1.0f),
			Vector3(1.0f, -1.0f, 1.0f),
			Vector3(-1.0f, -1.0f, 1.0f),

			Vector3(1.0f,  1.0f, 1.0f),
			Vector3(-1.0f, -1.0f, 1.0f),
			Vector3(-1.0f,  1.0f, 1.0f),

			Vector3(-1.0f,  1.0f,  1.0f),
			Vector3(-1.0f, -1.0f,  1.0f),
			Vector3(-1.0f, -1.0f, -1.0f),

			Vector3(-1.0f,  1.0f,  1.0f),
			Vector3(-1.0f, -1.0f, -1.0f),
			Vector3(-1.0f,  1.0f, -1.0f),

			Vector3(-1.0f, 1.0f,  1.0f),
			Vector3(-1.0f, 1.0f, -1.0f),
			Vector3(1.0f, 1.0f, -1.0f),

			Vector3(-1.0f, 1.0f,  1.0f),
			Vector3(1.0f, 1.0f, -1.0f),
			Vector3(1.0f, 1.0f,  1.0f),

			Vector3(-1.0f, -1.0f, -1.0f),
			Vector3(-1.0f, -1.0f,  1.0f),
			Vector3(1.0f, -1.0f,  1.0f),

			Vector3(-1.0f, -1.0f, -1.0f),
			Vector3(1.0f, -1.0f,  1.0f),
			Vector3(1.0f, -1.0f, -1.0f),
		};

		auto vertexCount = uint32_t(sizeof(vertices) / sizeof(vertices[0]));

		if (!m_VB.Init<Vector3>(pDevice, vertexCount, vertices))
		{
			ELOG("Error : VertexBuffer::Init() Failed.");
			return false;
		}
	}

	// 定数バッファの生成.
	{
		for (auto i = 0; i < 2; ++i)
		{
			if (!m_CB[i].Init(pDevice, m_pPoolRes, sizeof(CbSkyBox)))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");
				return false;
			}
		}
	}

	m_Index = 0;

	return true;
}

void SkyBox::Term()
{
	for (auto i = 0; i < 2; ++i)
	{
		m_CB[i].Term();
	}

	m_VB.Term();

	m_pPSO.Reset();
	m_RootSignature.Term();

	if (m_pPoolRes != nullptr)
	{
		m_pPoolRes->Release();
		m_pPoolRes = nullptr;
	}
}

void SkyBox::Draw(ID3D12GraphicsCommandList* pCmd, D3D12_GPU_DESCRIPTOR_HANDLE handle, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj, float boxSize)
{
	// 定数バッファの更新
	{
		auto ptr = m_CB[m_Index].GetPtr<CbSkyBox>();
		auto pos = Vector3(view._41, view._42, view._43);
		ptr->World = Matrix::CreateScale(boxSize) * Matrix::CreateTranslation(pos);
		ptr->View = view;
		ptr->Proj = proj;
	}

	auto vbv = m_VB.GetView();

	pCmd->SetGraphicsRootSignature(m_RootSignature.GetPtr());
	pCmd->SetGraphicsRootDescriptorTable(0, m_CB[m_Index].GetHandleGPU());
	pCmd->SetGraphicsRootDescriptorTable(1, handle);
	pCmd->SetPipelineState(m_pPSO.Get());
	pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCmd->IASetIndexBuffer(nullptr);
	pCmd->IASetVertexBuffers(0, 1, &vbv);
	pCmd->DrawInstanced(36, 1, 0, 0);

	// バッファ入れ替え.
	m_Index = (m_Index + 1) % 2;
}
