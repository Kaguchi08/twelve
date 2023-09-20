#include "Renderer.h"
#include "Dx12Wrapper.h"
#include "ModelComponent.h"
#include "FBXComponent.h"
#include "PrimitiveComponent.h"
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <string>
#include <algorithm>

Renderer::Renderer(class Dx12Wrapper* dx)
	: dx12_(dx)
{
}

Renderer::~Renderer()
{
}

void Renderer::Initialize()
{
	// ルートシグネチャの初期化
	CreatePMDModelRootSignature();
	CreateFBXModelRootSignature();
	CreatePrimitiveRootSignature();
	CreateScreenRootSignature();

	// パイプラインの初期化
	CreatePMDModelGraphicsPipeline();
	CreateFBXModelGraphicsPipeline();
	CreatePrimitiveGraphicsPipeline();
	CreateScreenGraphicsPipeline();

	// 汎用テクスチャの初期化
	white_texture_ = CreateWhiteTexture();
	black_texture_ = CreateBlackTexture();
	grad_texture_ = CreateGradTexture();
}

void Renderer::DrawToShadowMap()
{
	PrepareShadowMap();

	// PMD
	BeforeDrawPMDShadowMap();
	DrawPMDModel(true);

	// FBX
	BeforeDrawFBXShadowMap();
	DrawFBXModel(true);

	// Primitive
	BeforeDrawPrimitiveShadowMap();
	DrawPrimitive(true);
}

void Renderer::BeforePMDDraw()
{
	auto command_list = dx12_->GetCommandList();

	command_list->SetPipelineState(pmd_model_pipeline_state_.Get());
	command_list->SetGraphicsRootSignature(pmd_model_root_signature_.Get());
}

void Renderer::BeforeFBXDraw()
{
	auto command_list = dx12_->GetCommandList();

	command_list->SetPipelineState(fbx_model_pipeline_state_.Get());
	command_list->SetGraphicsRootSignature(fbx_model_root_signature_.Get());
}

void Renderer::BeforePrimitiveDraw()
{
	auto command_list = dx12_->GetCommandList();

	command_list->SetPipelineState(primitive_pipeline_state_.Get());
	command_list->SetGraphicsRootSignature(primitive_root_signature_.Get());
}

void Renderer::Draw(bool is_shadow)
{
	DrawPMDModel(is_shadow);
	DrawFBXModel(is_shadow);
}

void Renderer::AddModelComponent(ModelComponent* model)
{
	pmd_models_.emplace_back(model);
}

void Renderer::RemoveModelComponent(ModelComponent* model)
{
	auto it = std::find(pmd_models_.begin(), pmd_models_.end(), model);
	if (it != pmd_models_.end())
	{
		pmd_models_.erase(it);
	}
}

void Renderer::AddFBXComponent(FBXComponent* model)
{
	fbx_models_.emplace_back(model);
}

void Renderer::RemoveFBXComponent(FBXComponent* model)
{
	auto it = std::find(fbx_models_.begin(), fbx_models_.end(), model);
	if (it != fbx_models_.end())
	{
		fbx_models_.erase(it);
	}
}

void Renderer::AddPrimitiveComponent(PrimitiveComponent* primitive)
{
	primitives_.emplace_back(primitive);
}

void Renderer::RemovePrimitiveComponent(PrimitiveComponent* primitive)
{
	auto it = std::find(primitives_.begin(), primitives_.end(), primitive);
	if (it != primitives_.end())
	{
		primitives_.erase(it);
	}
}

ID3D12Resource* Renderer::CreateDefaultTexture(size_t width, size_t height)
{
	auto res_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	ID3D12Resource* tex_buffer = nullptr;
	auto result = dx12_->GetDevice()->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&tex_buffer)
	);

	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
		return nullptr;
	}

	return tex_buffer;
}

ID3D12Resource* Renderer::CreateWhiteTexture()
{
	ID3D12Resource* tex_buffer = CreateDefaultTexture(4, 4);

	// テクスチャの初期化
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);

	auto result = tex_buffer->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());
	assert(SUCCEEDED(result));
	return tex_buffer;
}

ID3D12Resource* Renderer::CreateBlackTexture()
{
	ID3D12Resource* tex_buffer = CreateDefaultTexture(4, 4);

	// テクスチャの初期化
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);

	auto result = tex_buffer->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());
	assert(SUCCEEDED(result));
	return tex_buffer;
}

ID3D12Resource* Renderer::CreateGradTexture()
{
	ID3D12Resource* tex_buffer = CreateDefaultTexture(4, 256);

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

	auto result = tex_buffer->WriteToSubresource(0, nullptr, data.data(), 4 * sizeof(unsigned int), sizeof(unsigned int) * data.size());
	assert(SUCCEEDED(result));
	return tex_buffer;
}

HRESULT Renderer::CreatePMDModelGraphicsPipeline()
{
	ComPtr<ID3DBlob> vs_blob = nullptr;
	ComPtr<ID3DBlob> ps_blob = nullptr;
	ComPtr<ID3DBlob> error_blob = nullptr;

	auto result = D3DCompileFromFile(
		L"PMDVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VSMain",
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
		L"PMDPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PSMain",
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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONENO",	  0, DXGI_FORMAT_R16G16_UINT,	  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHT",	  0, DXGI_FORMAT_R8_UINT,		  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
	gps_desc.pRootSignature = pmd_model_root_signature_.Get();

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(pmd_model_pipeline_state_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// 影
	result = D3DCompileFromFile(
		L"PMDVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VSShadow",
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

	gps_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	gps_desc.PS.BytecodeLength = 0; // PSなし
	gps_desc.PS.pShaderBytecode = nullptr; // PSなし
	gps_desc.NumRenderTargets = 0; // RTVなし
	gps_desc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN; // RTVなし

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(pmd_shadow_pipeline_state_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	return result;
}

HRESULT Renderer::CreateFBXModelGraphicsPipeline()
{
	ComPtr<ID3DBlob> vs_blob = nullptr;
	ComPtr<ID3DBlob> ps_blob = nullptr;
	ComPtr<ID3DBlob> error_blob = nullptr;

	auto result = D3DCompileFromFile(
		L"FBXVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",
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
		L"FBXPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",
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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
	gps_desc.pRootSignature = fbx_model_root_signature_.Get();

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(fbx_model_pipeline_state_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// 影
	result = result = D3DCompileFromFile(
		L"FBXVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VSShadow",
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

	gps_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	gps_desc.PS.BytecodeLength = 0; // PSなし
	gps_desc.PS.pShaderBytecode = nullptr; // PSなし
	gps_desc.NumRenderTargets = 0; // RTVなし
	gps_desc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN; // RTVなし

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(fbx_shadow_pipeline_state_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	return result;
}

HRESULT Renderer::CreatePrimitiveGraphicsPipeline()
{
	ComPtr<ID3DBlob> vs_blob = nullptr;
	ComPtr<ID3DBlob> ps_blob = nullptr;
	ComPtr<ID3DBlob> error_blob = nullptr;

	auto result = D3DCompileFromFile(
		L"PrimitiveVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VSMain",
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
		L"PrimitivePixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PSMain",
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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
	gps_desc.pRootSignature = primitive_root_signature_.Get();

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(primitive_pipeline_state_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// 影
	result = D3DCompileFromFile(
		L"PrimitiveVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VSShadow",
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

	gps_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	gps_desc.PS.BytecodeLength = 0; // PSなし
	gps_desc.PS.pShaderBytecode = nullptr; // PSなし
	gps_desc.NumRenderTargets = 0; // RTVなし
	gps_desc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN; // RTVなし

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(primitive_shadow_pipeline_state_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	return result;
}

HRESULT Renderer::CreateScreenGraphicsPipeline()
{
	ComPtr<ID3DBlob> vs_blob = nullptr;
	ComPtr<ID3DBlob> ps_blob = nullptr;
	ComPtr<ID3DBlob> error_blob = nullptr;

	auto result = D3DCompileFromFile(
		L"PeraVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PeraVS",
		"vs_5_0",
		0,
		0,
		vs_blob.ReleaseAndGetAddressOf(),
		error_blob.ReleaseAndGetAddressOf()
	);

	if (!(CheckShaderCompileResult(result, error_blob.Get())))
	{
		assert(0);
		return result;
	}

	result = D3DCompileFromFile(
		L"PeraPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PeraPS",
		"ps_5_0",
		0,
		0,
		ps_blob.ReleaseAndGetAddressOf(),
		error_blob.ReleaseAndGetAddressOf()
	);

	if (!(CheckShaderCompileResult(result, error_blob.Get())))
	{
		assert(0);
		return result;
	}

	D3D12_INPUT_ELEMENT_DESC input_elem_desc[2] = {
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gps_desc = {};
	gps_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	gps_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
	gps_desc.DepthStencilState.DepthEnable = false;
	gps_desc.DepthStencilState.StencilEnable = false;
	gps_desc.InputLayout.NumElements = _countof(input_elem_desc);
	gps_desc.InputLayout.pInputElementDescs = input_elem_desc;
	gps_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gps_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gps_desc.NumRenderTargets = 1;
	gps_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gps_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gps_desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gps_desc.SampleDesc.Count = 1;
	gps_desc.SampleDesc.Quality = 0;
	gps_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	gps_desc.pRootSignature = screen_root_signature_.Get();

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(
		&gps_desc,
		IID_PPV_ARGS(screen_pipeline_state_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	result = D3DCompileFromFile(
		L"PeraPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VerticalBokehPS",
		"ps_5_0",
		0,
		0,
		ps_blob.ReleaseAndGetAddressOf(),
		error_blob.ReleaseAndGetAddressOf()
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	gps_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(
		&gps_desc,
		IID_PPV_ARGS(screen_pipeline_state_2_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	return result;
}

HRESULT Renderer::CreatePMDModelRootSignature()
{
	const int num_range = 6;
	const int num_root_param = 5;
	const int num_samplers = 3;

	CD3DX12_DESCRIPTOR_RANGE range[num_range] = {};
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // scene_cbv
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // transform_cbv
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // material_cbv
	range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);
	range[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3); // light_cbv
	range[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4); // シャドウマップ

	CD3DX12_ROOT_PARAMETER root_param[num_root_param] = {};
	root_param[0].InitAsDescriptorTable(1, &range[0]);
	root_param[1].InitAsDescriptorTable(1, &range[1]);
	root_param[2].InitAsDescriptorTable(2, &range[2]); // マテリアル関連はまとめる
	root_param[3].InitAsDescriptorTable(1, &range[4]);
	root_param[4].InitAsDescriptorTable(1, &range[5]);

	CD3DX12_STATIC_SAMPLER_DESC sampler_desc[num_samplers] = {};
	sampler_desc[0].Init(0);
	sampler_desc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	sampler_desc[2].Init(2);
	sampler_desc[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	sampler_desc[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc[2].MaxAnisotropy = 1;
	sampler_desc[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc = {};
	root_signature_desc.Init(num_root_param, root_param, num_samplers, sampler_desc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> root_sig_blob;
	ComPtr<ID3DBlob> error_blob;

	auto result = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_sig_blob, &error_blob);
	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
		return result;
	}

	result = dx12_->GetDevice()->CreateRootSignature(0, root_sig_blob->GetBufferPointer(), root_sig_blob->GetBufferSize(), IID_PPV_ARGS(pmd_model_root_signature_.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
		return result;
	}

	return result;
}

HRESULT Renderer::CreateFBXModelRootSignature()
{
	const int num_ranges = 7;
	const int num_root_params = 7;
	const int num_samplers = 2;

	CD3DX12_DESCRIPTOR_RANGE range[num_ranges] = {};

	// 定数
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // シーン関連 
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // マテリアル関連
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // シャドウマップ用
	range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // テクスチャ
	range[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // ワールド行列
	range[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // 法線マップ
	range[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); // ARMマップ


	CD3DX12_ROOT_PARAMETER root_param[num_root_params] = {};

	root_param[0].InitAsDescriptorTable(1, &range[0]);
	root_param[1].InitAsDescriptorTable(1, &range[1]);
	root_param[2].InitAsDescriptorTable(1, &range[2]);
	root_param[3].InitAsDescriptorTable(1, &range[3]);
	root_param[4].InitAsDescriptorTable(1, &range[4]);
	root_param[5].InitAsDescriptorTable(1, &range[5]);
	root_param[6].InitAsDescriptorTable(1, &range[6]);

	CD3DX12_STATIC_SAMPLER_DESC sampler_desc[num_samplers] = {};

	sampler_desc[0].Init(0);
	sampler_desc[1].Init(1);
	sampler_desc[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	sampler_desc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc[1].MaxAnisotropy = 1;
	sampler_desc[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc = {};

	root_signature_desc.Init(num_root_params, root_param, num_samplers, sampler_desc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> root_sig_blob;
	ComPtr<ID3DBlob> error_blob;

	auto result = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_sig_blob, &error_blob);
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	result = dx12_->GetDevice()->CreateRootSignature(0, root_sig_blob->GetBufferPointer(), root_sig_blob->GetBufferSize(), IID_PPV_ARGS(fbx_model_root_signature_.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	return result;
}

HRESULT Renderer::CreatePrimitiveRootSignature()
{
	const int num_ranges = 8;
	const int num_root_params = 8;
	const int num_samplers = 2;

	CD3DX12_DESCRIPTOR_RANGE range[num_ranges] = {};

	// 定数
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // シーン関連 
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // マテリアル関連
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // テクスチャ
	range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // ライト
	range[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // 法線マップ
	range[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // ARMマップ
	range[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); // シャドウマップ
	range[7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3); // ワールド行列


	CD3DX12_ROOT_PARAMETER root_param[num_root_params] = {};

	root_param[0].InitAsDescriptorTable(1, &range[0]);
	root_param[1].InitAsDescriptorTable(1, &range[1]);
	root_param[2].InitAsDescriptorTable(1, &range[2]);
	root_param[3].InitAsDescriptorTable(1, &range[3]);
	root_param[4].InitAsDescriptorTable(1, &range[4]);
	root_param[5].InitAsDescriptorTable(1, &range[5]);
	root_param[6].InitAsDescriptorTable(1, &range[6]);
	root_param[7].InitAsDescriptorTable(1, &range[7]);

	CD3DX12_STATIC_SAMPLER_DESC sampler_desc[num_samplers] = {};

	sampler_desc[0].Init(0);
	sampler_desc[1].Init(1);
	sampler_desc[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	sampler_desc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc[1].MaxAnisotropy = 1;
	sampler_desc[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc = {};

	root_signature_desc.Init(num_root_params, root_param, num_samplers, sampler_desc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> root_sig_blob;
	ComPtr<ID3DBlob> error_blob;

	auto result = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_sig_blob, &error_blob);
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	result = dx12_->GetDevice()->CreateRootSignature(0, root_sig_blob->GetBufferPointer(), root_sig_blob->GetBufferSize(), IID_PPV_ARGS(primitive_root_signature_.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	return result;
}

HRESULT Renderer::CreateScreenRootSignature()
{
	D3D12_DESCRIPTOR_RANGE range[4] = {};

	// ガウスウェイト
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // b0
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;

	// テクスチャ
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // t0
	range[1].BaseShaderRegister = 0;
	range[1].NumDescriptors = 1;

	// エフェクト
	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // t1
	range[2].BaseShaderRegister = 1;
	range[2].NumDescriptors = 1;

	// 深度値テクスチャ
	range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // t2
	range[3].BaseShaderRegister = 2;
	range[3].NumDescriptors = 1;

	D3D12_ROOT_PARAMETER root_param[4] = {};

	root_param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	root_param[0].DescriptorTable.pDescriptorRanges = &range[0];
	root_param[0].DescriptorTable.NumDescriptorRanges = 1;

	root_param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	root_param[1].DescriptorTable.pDescriptorRanges = &range[1];
	root_param[1].DescriptorTable.NumDescriptorRanges = 1;

	root_param[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_param[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	root_param[2].DescriptorTable.pDescriptorRanges = &range[2];
	root_param[2].DescriptorTable.NumDescriptorRanges = 1;

	root_param[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_param[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	root_param[3].DescriptorTable.pDescriptorRanges = &range[3];
	root_param[3].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC sampler_desc = CD3DX12_STATIC_SAMPLER_DESC(0);
	sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	D3D12_ROOT_SIGNATURE_DESC root_sig_desc = {};
	root_sig_desc.NumParameters = 4;
	root_sig_desc.pParameters = root_param;
	root_sig_desc.NumStaticSamplers = 1;
	root_sig_desc.pStaticSamplers = &sampler_desc;
	root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> root_sig_blob = nullptr;
	ComPtr<ID3DBlob> error_blob = nullptr;

	auto result = D3D12SerializeRootSignature(
		&root_sig_desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		root_sig_blob.ReleaseAndGetAddressOf(),
		error_blob.ReleaseAndGetAddressOf()
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	result = dx12_->GetDevice()->CreateRootSignature(
		0,
		root_sig_blob->GetBufferPointer(),
		root_sig_blob->GetBufferSize(),
		IID_PPV_ARGS(screen_root_signature_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	return result;
}

void Renderer::DrawPMDModel(bool is_shadow)
{
	for (auto& model : pmd_models_)
	{
		model->DrawPMD(is_shadow);
	}
}

void Renderer::DrawFBXModel(bool is_shadow)
{
	for (auto& model : fbx_models_)
	{
		model->Draw(is_shadow);
	}
}

void Renderer::DrawPrimitive(bool is_shadow)
{
	for (auto& primitive : primitives_)
	{
		primitive->Draw(is_shadow);
	}
}

void Renderer::PrepareShadowMap()
{
	auto cmd_list = dx12_->GetCommandList();

	auto handle = dx12_->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	cmd_list->OMSetRenderTargets(0, nullptr, false, &handle);

	cmd_list->ClearDepthStencilView(handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	D3D12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, 1024, 1024);
	cmd_list->RSSetViewports(1, &viewport);

	D3D12_RECT scissor = CD3DX12_RECT(0, 0, 1024, 1024);
	cmd_list->RSSetScissorRects(1, &scissor);
}

void Renderer::BeforeDrawPMDShadowMap()
{
	auto cmd_list = dx12_->GetCommandList();

	cmd_list->SetPipelineState(pmd_shadow_pipeline_state_.Get());
	cmd_list->SetGraphicsRootSignature(pmd_model_root_signature_.Get());

	ID3D12DescriptorHeap* heaps[] = { dx12_->GetSceneCBVHeap().Get() };

	cmd_list->SetDescriptorHeaps(1, heaps);
	cmd_list->SetGraphicsRootDescriptorTable(0, dx12_->GetSceneCBVHeap()->GetGPUDescriptorHandleForHeapStart());
}

void Renderer::BeforeDrawFBXShadowMap()
{
	auto cmd_list = dx12_->GetCommandList();

	cmd_list->SetPipelineState(fbx_shadow_pipeline_state_.Get());
	cmd_list->SetGraphicsRootSignature(fbx_model_root_signature_.Get());

	ID3D12DescriptorHeap* heaps[] = { dx12_->GetSceneCBVHeap().Get() };

	cmd_list->SetDescriptorHeaps(1, heaps);
	cmd_list->SetGraphicsRootDescriptorTable(0, dx12_->GetSceneCBVHeap()->GetGPUDescriptorHandleForHeapStart());
}

void Renderer::BeforeDrawPrimitiveShadowMap()
{
	auto cmd_list = dx12_->GetCommandList();

	cmd_list->SetPipelineState(primitive_shadow_pipeline_state_.Get());
	cmd_list->SetGraphicsRootSignature(primitive_root_signature_.Get());

	ID3D12DescriptorHeap* heaps[] = { dx12_->GetSceneCBVHeap().Get() };

	cmd_list->SetDescriptorHeaps(1, heaps);
	cmd_list->SetGraphicsRootDescriptorTable(0, dx12_->GetSceneCBVHeap()->GetGPUDescriptorHandleForHeapStart());
}

bool Renderer::CheckShaderCompileResult(HRESULT result, ID3DBlob* error)
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
