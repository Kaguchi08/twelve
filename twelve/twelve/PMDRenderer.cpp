#include "PMDRenderer.h"
#include <d3dx12.h>
#include <cassert>
#include <d3dcompiler.h>
#include "Dx12Wrapper.h"
#include <string>
#include <algorithm>

namespace
{
	void PrintErrorBlob(ID3DBlob* blob)
	{
		assert(blob);
		std::string err;
		err.resize(blob->GetBufferSize());
		copy_n((char*)blob->GetBufferPointer(), blob->GetBufferSize(), err.begin());
	}
}

PMDRenderer::PMDRenderer(Dx12Wrapper& dx12)
	: dx12_(dx12)
{
}

PMDRenderer::~PMDRenderer()
{
}

void PMDRenderer::Initialize()
{
	CreateRootSignature();
	CreateGraphicsPipeline();
	// テクスチャの生成
	mWhiteTexture = CreateWhiteTexture();
	mBlackTexture = CreateBlackTexture();
	mGradTexture = CreateGradTexture();
}

void PMDRenderer::BeforePMDDraw()
{
	auto cmd_list = dx12_.GetCommandList();
	cmd_list->SetPipelineState(pipeline_state_.Get());
	cmd_list->SetGraphicsRootSignature(root_signature_.Get());
}

ID3D12Resource* PMDRenderer::CreateDefaultTexture(size_t width, size_t height)
{
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	ID3D12Resource* texBuff = nullptr;
	auto result = dx12_.GetDevice()->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texBuff)
	);

	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
		return nullptr;
	}

	return texBuff;
}

ID3D12Resource* PMDRenderer::CreateWhiteTexture()
{
	ID3D12Resource* whiteTexBuff = CreateDefaultTexture(4, 4);

	// テクスチャの初期化
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);

	auto result = whiteTexBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());
	assert(SUCCEEDED(result));
	return whiteTexBuff;
}

ID3D12Resource* PMDRenderer::CreateBlackTexture()
{
	ID3D12Resource* blackTexBuff = CreateDefaultTexture(4, 4);

	// テクスチャの初期化
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);

	auto result = blackTexBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());
	assert(SUCCEEDED(result));
	return blackTexBuff;
}

// 修正の余地あり
ID3D12Resource* PMDRenderer::CreateGradTexture()
{
	ID3D12Resource* gradTexBuff = CreateDefaultTexture(4, 256);

	// 上が白くて下が黒いテクスチャデータの作成
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4)
	{
		auto col = (0xff << 24) | RGB(c, c, c);
		std::fill(it, it + 4, col);
		--c;
	}

	auto result = gradTexBuff->WriteToSubresource(0, nullptr, data.data(), 4 * sizeof(unsigned int), sizeof(unsigned int) * data.size());
	assert(SUCCEEDED(result));
	return gradTexBuff;
}

HRESULT PMDRenderer::CreateGraphicsPipeline()
{
	ComPtr<ID3DBlob> vs_blob = nullptr;
	ComPtr<ID3DBlob> ps_blob = nullptr;
	ComPtr<ID3DBlob> error_blob = nullptr;

	auto result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&vs_blob,
		&error_blob
	);

	if (!(CheckShaderCompileResult(result, error_blob.Get())))
	{
		assert(0);
		return result;
	}

	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&ps_blob,
		&error_blob
	);

	if (!(CheckShaderCompileResult(result, error_blob.Get())))
	{
		assert(0);
		return result;
	}

	// 頂点レイアウトの定義
	D3D12_INPUT_ELEMENT_DESC input_elem_desc[] =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONENO",		0, DXGI_FORMAT_R16G16_UINT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHT",		0, DXGI_FORMAT_R8_UINT,			0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "EDGE_FLG",	0, DXGI_FORMAT_R8_UINT,			0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gps_desc = {};

	gps_desc.InputLayout.pInputElementDescs = input_elem_desc;
	gps_desc.InputLayout.NumElements = _countof(input_elem_desc);
	gps_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gps_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	// シェーダ
	gps_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	gps_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());

	// ラスタライザ
	gps_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gps_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// 出力
	gps_desc.NumRenderTargets = 1;
	gps_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	gps_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gps_desc.BlendState.RenderTarget[0].BlendEnable = true;
	gps_desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	gps_desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	gps_desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	gps_desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	gps_desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	gps_desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;

	// 深度ステンシル
	gps_desc.DepthStencilState.DepthEnable = true;
	gps_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gps_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gps_desc.DepthStencilState.StencilEnable = false;
	gps_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	// 全体的な設定
	gps_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	gps_desc.NodeMask = 0;
	gps_desc.SampleDesc.Count = 1;
	gps_desc.SampleDesc.Quality = 0;
	gps_desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gps_desc.pRootSignature = root_signature_.Get();

	result = dx12_.GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(pipeline_state_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
	}

	return result;
}

HRESULT PMDRenderer::CreateRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE range[4] = {};
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

	CD3DX12_ROOT_PARAMETER root_param[3] = {};
	root_param[0].InitAsDescriptorTable(1, &range[0]);
	root_param[1].InitAsDescriptorTable(1, &range[1]);
	root_param[2].InitAsDescriptorTable(2, &range[2]);

	CD3DX12_STATIC_SAMPLER_DESC sampler_desc[2] = {};
	sampler_desc[0].Init(0);
	sampler_desc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc = {};
	root_signature_desc.Init(3, root_param, 2, sampler_desc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> root_sig_blob;
	ComPtr<ID3DBlob> error_blob;

	auto result = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_sig_blob, &error_blob);
	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
		return result;
	}

	result = dx12_.GetDevice()->CreateRootSignature(0, root_sig_blob->GetBufferPointer(), root_sig_blob->GetBufferSize(), IID_PPV_ARGS(root_signature_.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
		return result;
	}

	return result;
}

bool PMDRenderer::CheckShaderCompileResult(HRESULT result, ID3DBlob* error)
{
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			OutputDebugStringA("ファイルが見つかりません");
		}
		else
		{
			std::string errstr;
			errstr.resize(error->GetBufferSize());
			std::copy_n((char*)error->GetBufferPointer(), error->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		return false;
	}
	return true;
}
