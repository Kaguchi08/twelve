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
	// �����_�[�^�[�Q�b�g�̏�����
	CreateRenderTarget();

	CreateOffScreenResourceAndView();

	// ���[�g�V�O�l�`���̏�����
	CreatePMDModelRootSignature();
	CreateFBXModelRootSignature();
	CreatePrimitiveRootSignature();
	CreateDeferredLightingRootSignature();
	CreateScreenRootSignature();

	// �p�C�v���C���̏�����
	CreatePMDModelGraphicsPipeline();
	CreateFBXModelGraphicsPipeline();
	CreatePrimitiveGraphicsPipeline();
	CreateDefferedLightingGraphicsPipeline();
	CreateScreenGraphicsPipeline();

	// �ėp�e�N�X�`���̏�����
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

void Renderer::DrawToZPrepass()
{
	PrepareZPrepass();

	// FBX
	BeforeDrawFBXZPrepass();
	DrawFBXModel(true);

	// Primitive
	BeforeDrawPrimitiveZPrepass();
	DrawPrimitive(true);
}

void Renderer::DrawToGBuffer()
{
	PrepareGBuffer();

	// FBX
	BeforeDrawFBXGBuffer();
	dx12_->SetCommonBuffer(0, 7, 2);
	DrawFBXModel(false);

	// Primitive
	BeforeDrawPrimitiveGBuffer();
	dx12_->SetCommonBuffer(0, 3, 6);
	DrawPrimitive(false);

	EndDrawGBuffer();
}

void Renderer::DrawDeferredLighting()
{
	BarrierTransResource(deferred_lighting_render_target.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto cmd_list = dx12_->GetCommandList();
	auto rtv_handle = off_screen_rtv_heap_->GetCPUDescriptorHandleForHeapStart();

	// �����_�[�^�[�Q�b�g�̐ݒ�
	cmd_list->OMSetRenderTargets(1, &rtv_handle, false, nullptr);

	// �����_�[�^�[�Q�b�g�̃N���A
	float clear_color[] = { 0.5f, 0.5f, 0.5f, 1.0f };

	cmd_list->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);

	cmd_list->RSSetViewports(1, view_port_.get());
	cmd_list->RSSetScissorRects(1, scissor_rect_.get());

	// ���[�g�V�O�l�`���̐ݒ�
	cmd_list->SetGraphicsRootSignature(deferred_lighting_root_signature_.Get());
	// �p�C�v���C���X�e�[�g�̐ݒ�
	cmd_list->SetPipelineState(deferred_lighting_pipeline_state_.Get());

	// ���ʃo�b�t�@��ݒ�
	dx12_->SetCommonBuffer(0, 1, 6);

	// G-Buffer�ɏo�͂��ꂽ�e�X�N�`���̐ݒ�
	// �f�B�X�N���v�^�q�[�v�̐ݒ�
	cmd_list->SetDescriptorHeaps(1, off_sceen_srv_heap_.GetAddressOf());

	auto handle = off_sceen_srv_heap_->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += 2 * dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < kNumGBuffer; ++i)
	{
		cmd_list->SetGraphicsRootDescriptorTable(i + 2, handle); // 0, 1�͒萔�o�b�t�@
		handle.ptr += dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	D3D12_VERTEX_BUFFER_VIEW view = dx12_->GetScreenVertexBufferView();
	cmd_list->IASetVertexBuffers(0, 1, &view);
	cmd_list->DrawInstanced(4, 1, 0, 0);

	BarrierTransResource(deferred_lighting_render_target.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
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

	// �e�N�X�`���̏�����
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);

	auto result = tex_buffer->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());
	assert(SUCCEEDED(result));
	return tex_buffer;
}

ID3D12Resource* Renderer::CreateBlackTexture()
{
	ID3D12Resource* tex_buffer = CreateDefaultTexture(4, 4);

	// �e�N�X�`���̏�����
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);

	auto result = tex_buffer->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());
	assert(SUCCEEDED(result));
	return tex_buffer;
}

ID3D12Resource* Renderer::CreateGradTexture()
{
	ID3D12Resource* tex_buffer = CreateDefaultTexture(4, 256);

	// �オ�����ĉ��������e�N�X�`���f�[�^�̍쐬
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

	// ���_���C�A�E�g�̒�`
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

	// �V�F�[�_
	gps_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	gps_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());

	// ���X�^���C�U
	gps_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gps_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// �o��
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

	// �[�x�X�e���V��
	gps_desc.DepthStencilState.DepthEnable = true;
	gps_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gps_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gps_desc.DepthStencilState.StencilEnable = false;
	gps_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	// �S�̓I�Ȑݒ�
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

	// �e
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
	gps_desc.PS.BytecodeLength = 0; // PS�Ȃ�
	gps_desc.PS.pShaderBytecode = nullptr; // PS�Ȃ�
	gps_desc.NumRenderTargets = 0; // RTV�Ȃ�
	gps_desc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN; // RTV�Ȃ�

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

	// ���_���C�A�E�g�̒�`
	D3D12_INPUT_ELEMENT_DESC input_elem_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gps_desc = {};

	gps_desc.InputLayout.pInputElementDescs = input_elem_desc;
	gps_desc.InputLayout.NumElements = _countof(input_elem_desc);
	gps_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gps_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	// �V�F�[�_
	gps_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	gps_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());

	// ���X�^���C�U
	gps_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gps_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// �o��
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

	// �[�x�X�e���V��
	gps_desc.DepthStencilState.DepthEnable = true;
	gps_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gps_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gps_desc.DepthStencilState.StencilEnable = false;
	gps_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	// �S�̓I�Ȑݒ�
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

	// �e
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
	gps_desc.PS.BytecodeLength = 0; // PS�Ȃ�
	gps_desc.PS.pShaderBytecode = nullptr; // PS�Ȃ�
	gps_desc.NumRenderTargets = 0; // RTV�Ȃ�
	gps_desc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN; // RTV�Ȃ�

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(fbx_shadow_pipeline_state_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// Zprepass
	result = result = D3DCompileFromFile(
		L"FBXVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VSZPrepass",
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

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(fbx_zprepass_pipeline_state_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// G-Buffer
	result = result = D3DCompileFromFile(
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

	result = result = D3DCompileFromFile(
		L"FBXPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PSGBuffer",
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

	gps_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	gps_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());

	gps_desc.NumRenderTargets = kNumGBuffer;
	gps_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // �A���x�h
	gps_desc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM; // �@��
	gps_desc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM; // ���[���h���W
	gps_desc.RTVFormats[3] = DXGI_FORMAT_R8G8B8A8_UNORM; // �����x + ���炩��

	// ZPrepass�ŏ������񂾐[�x�o�b�t�@���g��
	gps_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // �ǂݎ���p
	gps_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(fbx_gbuffer_pipeline_state_.ReleaseAndGetAddressOf()));

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

	// ���_���C�A�E�g�̒�`
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

	// �V�F�[�_
	gps_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	gps_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());

	// ���X�^���C�U
	gps_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gps_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// �o��
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

	// �[�x�X�e���V��
	gps_desc.DepthStencilState.DepthEnable = true;
	gps_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gps_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gps_desc.DepthStencilState.StencilEnable = false;
	gps_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	// �S�̓I�Ȑݒ�
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

	// �e
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
	gps_desc.PS.BytecodeLength = 0; // PS�Ȃ�
	gps_desc.PS.pShaderBytecode = nullptr; // PS�Ȃ�
	gps_desc.NumRenderTargets = 0; // RTV�Ȃ�
	gps_desc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN; // RTV�Ȃ�

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(primitive_shadow_pipeline_state_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// Zprepass
	result = result = D3DCompileFromFile(
		L"PrimitiveVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VSZPrepass",
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

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(primitive_zprepass_pipeline_state_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// G-Buffer
	result = result = D3DCompileFromFile(
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

	result = result = D3DCompileFromFile(
		L"PrimitivePixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PSGBuffer",
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

	gps_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	gps_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());

	gps_desc.NumRenderTargets = kNumGBuffer;
	gps_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // �A���x�h
	gps_desc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM; // �@��
	gps_desc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM; // ���[���h���W
	gps_desc.RTVFormats[3] = DXGI_FORMAT_R8G8B8A8_UNORM; // �����x + ���炩��

	// ZPrepass�ŏ������񂾐[�x�o�b�t�@���g��
	gps_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // �ǂݎ���p
	gps_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(primitive_gbuffer_pipeline_state_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	return result;
}

HRESULT Renderer::CreateDefferedLightingGraphicsPipeline()
{
	ComPtr<ID3DBlob> vs_blob = nullptr;
	ComPtr<ID3DBlob> ps_blob = nullptr;
	ComPtr<ID3DBlob> error_blob = nullptr;

	auto result = D3DCompileFromFile(
		L"DefferedLighting.hlsl",
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
		L"DefferedLighting.hlsl",
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

	// ���_���C�A�E�g�̒�`
	D3D12_INPUT_ELEMENT_DESC input_elem_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gps_desc = {};

	gps_desc.InputLayout.pInputElementDescs = input_elem_desc;
	gps_desc.InputLayout.NumElements = _countof(input_elem_desc);
	gps_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gps_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	// �V�F�[�_
	gps_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	gps_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());

	// ���X�^���C�U
	gps_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gps_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// �o��
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

	// �[�x�X�e���V��
	gps_desc.DepthStencilState.DepthEnable = false;
	gps_desc.DepthStencilState.StencilEnable = false;

	// �S�̓I�Ȑݒ�
	gps_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	gps_desc.NodeMask = 0;
	gps_desc.SampleDesc.Count = 1;
	gps_desc.SampleDesc.Quality = 0;
	gps_desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gps_desc.pRootSignature = deferred_lighting_root_signature_.Get();

	result = dx12_->GetDevice()->CreateGraphicsPipelineState(&gps_desc, IID_PPV_ARGS(deferred_lighting_pipeline_state_.ReleaseAndGetAddressOf()));

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
	range[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4); // �V���h�E�}�b�v

	CD3DX12_ROOT_PARAMETER root_param[num_root_param] = {};
	root_param[0].InitAsDescriptorTable(1, &range[0]);
	root_param[1].InitAsDescriptorTable(1, &range[1]);
	root_param[2].InitAsDescriptorTable(2, &range[2]); // �}�e���A���֘A�͂܂Ƃ߂�
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
	const int num_ranges = 8;
	const int num_root_params = 8;
	const int num_samplers = 2;

	CD3DX12_DESCRIPTOR_RANGE range[num_ranges] = {};

	// �萔
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // �V�[���֘A 
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // �}�e���A���֘A
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // �V���h�E�}�b�v�p
	range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // �e�N�X�`��
	range[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // ���[���h�s��
	range[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // �@���}�b�v
	range[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); // ARM�}�b�v
	range[7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3); // ���C�g


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

	// �萔
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // �V�[���֘A 
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // �}�e���A���֘A
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // �e�N�X�`��
	range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // ���C�g
	range[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // �@���}�b�v
	range[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // ARM�}�b�v
	range[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); // �V���h�E�}�b�v
	range[7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3); // ���[���h�s��


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

HRESULT Renderer::CreateDeferredLightingRootSignature()
{
	const int num_ranges = 7;
	const int num_root_params = 7;
	const int num_samplers = 2;

	CD3DX12_DESCRIPTOR_RANGE range[num_ranges] = {};

	// �萔
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // �V�[���֘A 
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // ���C�g
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // �A���x�h
	range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // �@��
	range[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // ���[���h���W
	range[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); // ARM�}�b�v
	range[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4); // �V���h�E�}�b�v

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

	result = dx12_->GetDevice()->CreateRootSignature(0, root_sig_blob->GetBufferPointer(), root_sig_blob->GetBufferSize(), IID_PPV_ARGS(deferred_lighting_root_signature_.ReleaseAndGetAddressOf()));
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

	// �K�E�X�E�F�C�g
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // b0
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;

	// �e�N�X�`��
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // t0
	range[1].BaseShaderRegister = 0;
	range[1].NumDescriptors = 1;

	// �G�t�F�N�g
	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // t1
	range[2].BaseShaderRegister = 1;
	range[2].NumDescriptors = 1;

	// �[�x�l�e�N�X�`��
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

HRESULT Renderer::CreateRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	auto s = dx12_->GetSwapChain();

	auto result = s->GetDesc1(&swap_chain_desc);

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heap_desc.NodeMask = 0;
	heap_desc.NumDescriptors = swap_chain_desc.BufferCount;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = dx12_->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(rtv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	render_targets_.resize(swap_chain_desc.BufferCount);

	auto handle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
	auto inc_size = dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	for (size_t i = 0; i < swap_chain_desc.BufferCount; ++i)
	{
		result = dx12_->GetSwapChain()->GetBuffer(i, IID_PPV_ARGS(&render_targets_[i]));
		assert(SUCCEEDED(result));
		if (FAILED(result))
		{
			assert(0);
			return result;
		}

		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // �K���}�␳������ꍇ��DXGI_FORMAT_R8G8B8A8_UNORM_SRGB���w�肷��
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		dx12_->GetDevice()->CreateRenderTargetView(render_targets_[i], &rtv_desc, handle);

		handle.ptr += inc_size;
	}


	SIZE window_size = dx12_->GetWindowSize();
	view_port_.reset(new CD3DX12_VIEWPORT(0.0f, 0.0f, window_size.cx, window_size.cy));
	scissor_rect_.reset(new CD3DX12_RECT(0.0f, 0.0f, window_size.cx, window_size.cy));

	return result;
}

bool Renderer::CreateOffScreenResourceAndView()
{
	// �����_�[�^�[�Q�b�g�̍쐬
	// �g���Ă��郌���_�[�^�[�Q�b�g�̏��𗘗p
	auto& b_buff = render_targets_[0];
	auto res_desc = b_buff->GetDesc();

	D3D12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	float clear_color[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	D3D12_CLEAR_VALUE clear_value = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clear_color);

	// �f�B�t�@�[�h���C�e�B���O�p
	auto result = dx12_->GetDevice()->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clear_value,
		IID_PPV_ARGS(deferred_lighting_render_target.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// �|�X�g�G�t�F�N�g�p
	result = dx12_->GetDevice()->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clear_value,
		IID_PPV_ARGS(post_effect_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// G-Buffer�p
	for (auto& g_buffer : g_buffers_)
	{
		result = dx12_->GetDevice()->CreateCommittedResource(
			&heap_prop,
			D3D12_HEAP_FLAG_NONE,
			&res_desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clear_value,
			IID_PPV_ARGS(g_buffer.ReleaseAndGetAddressOf())
		);

		if (FAILED(result))
		{
			assert(0);
			return false;
		}
	}

	// �f�B�X�N���v�^�q�[�v�̍쐬�iRTV�j
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heap_desc.NodeMask = 0;
	heap_desc.NumDescriptors = 2 + kNumGBuffer; // �f�B�t�@�[�h���C�e�B���O�p + �|�X�g�G�t�F�N�g�p + G-Buffer�p
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = dx12_->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(off_screen_rtv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// RTV�쐬
	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	auto handle = off_screen_rtv_heap_->GetCPUDescriptorHandleForHeapStart();

	// �f�B�t�@�[�h���C�e�B���O�p
	dx12_->GetDevice()->CreateRenderTargetView(deferred_lighting_render_target.Get(), &rtv_desc, handle);
	handle.ptr += dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// �|�X�g�G�t�F�N�g�p
	dx12_->GetDevice()->CreateRenderTargetView(post_effect_.Get(), &rtv_desc, handle);
	handle.ptr += dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// G-Buffer�p
	for (auto& g_buffer : g_buffers_)
	{
		dx12_->GetDevice()->CreateRenderTargetView(g_buffer.Get(), &rtv_desc, handle);
		handle.ptr += dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// �f�B�X�N���v�^�q�[�v�̍쐬�iSRV�j
	heap_desc = {};
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.NodeMask = 0;
	heap_desc.NumDescriptors = 2 + kNumGBuffer; // �f�B�t�@�[�h���C�e�B���O�p + �|�X�g�G�t�F�N�g�p + G-Buffer�p
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	result = dx12_->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(off_sceen_srv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// SRV�쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	handle = off_sceen_srv_heap_->GetCPUDescriptorHandleForHeapStart();

	// �f�B�t�@�[�h���C�e�B���O�p
	dx12_->GetDevice()->CreateShaderResourceView(deferred_lighting_render_target.Get(), &srv_desc, handle);
	handle.ptr += dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// �|�X�g�G�t�F�N�g�p
	dx12_->GetDevice()->CreateShaderResourceView(post_effect_.Get(), &srv_desc, handle);
	handle.ptr += dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// G-Buffer�p
	for (auto& g_buffer : g_buffers_)
	{
		dx12_->GetDevice()->CreateShaderResourceView(g_buffer.Get(), &srv_desc, handle);
		handle.ptr += dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	return true;
}

void Renderer::BarrierTransResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
	dx12_->GetCommandList()->ResourceBarrier(1, &barrier);
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

bool Renderer::PreDrawToPera()
{
	BarrierTransResource(deferred_lighting_render_target.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto rtvH = off_screen_rtv_heap_->GetCPUDescriptorHandleForHeapStart();
	auto dsvH = dx12_->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();
	dx12_->GetCommandList()->OMSetRenderTargets(1, &rtvH, false, &dsvH);

	/*float clear_color[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	dx12_->GetCommandList()->ClearRenderTargetView(rtvH, clear_color, 0, nullptr);*/

	dx12_->GetCommandList()->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	return true;
}

void Renderer::DrawToPera1()
{
	dx12_->GetCommandList()->RSSetViewports(1, view_port_.get());
	dx12_->GetCommandList()->RSSetScissorRects(1, scissor_rect_.get());
}

void Renderer::DrawToPera2()
{
	auto cmd_list = dx12_->GetCommandList();

	BarrierTransResource(post_effect_.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto rtvH = off_screen_rtv_heap_->GetCPUDescriptorHandleForHeapStart();

	rtvH.ptr += dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	cmd_list->OMSetRenderTargets(1, &rtvH, false, nullptr);

	cmd_list->RSSetViewports(1, view_port_.get());
	cmd_list->RSSetScissorRects(1, scissor_rect_.get());

	cmd_list->SetGraphicsRootSignature(screen_root_signature_.Get());

	cmd_list->SetDescriptorHeaps(1, off_sceen_srv_heap_.GetAddressOf());
	auto handle = off_sceen_srv_heap_->GetGPUDescriptorHandleForHeapStart();
	cmd_list->SetGraphicsRootDescriptorTable(1, handle);

	cmd_list->SetDescriptorHeaps(1, dx12_->GetPeraCBVHeap().GetAddressOf());
	handle = dx12_->GetPeraCBVHeap()->GetGPUDescriptorHandleForHeapStart();
	cmd_list->SetGraphicsRootDescriptorTable(0, handle);

	cmd_list->SetDescriptorHeaps(1, dx12_->GetDepthSRVHeap().GetAddressOf());
	handle = dx12_->GetDepthSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	cmd_list->SetGraphicsRootDescriptorTable(3, handle);

	cmd_list->SetPipelineState(screen_pipeline_state_.Get());
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	D3D12_VERTEX_BUFFER_VIEW view = dx12_->GetScreenVertexBufferView();
	cmd_list->IASetVertexBuffers(0, 1, &view);
	cmd_list->DrawInstanced(4, 1, 0, 0);

	BarrierTransResource(post_effect_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

bool Renderer::Clear()
{
	BarrierTransResource(deferred_lighting_render_target.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	auto bbIdx = dx12_->GetSwapChain()->GetCurrentBackBufferIndex();

	BarrierTransResource(render_targets_[bbIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto rtvH = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	dx12_->GetCommandList()->OMSetRenderTargets(1, &rtvH, false, nullptr);

	float clear_color[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	dx12_->GetCommandList()->ClearRenderTargetView(rtvH, clear_color, 0, nullptr);

	return true;
}

void Renderer::DrawToBackBuffer()
{
	auto cmd_list = dx12_->GetCommandList();

	cmd_list->RSSetViewports(1, view_port_.get());
	cmd_list->RSSetScissorRects(1, scissor_rect_.get());

	cmd_list->SetGraphicsRootSignature(screen_root_signature_.Get());
	cmd_list->SetDescriptorHeaps(1, off_sceen_srv_heap_.GetAddressOf());

	auto handle = off_sceen_srv_heap_->GetGPUDescriptorHandleForHeapStart();
	cmd_list->SetGraphicsRootDescriptorTable(0, handle);
	handle.ptr += dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cmd_list->SetGraphicsRootDescriptorTable(1, handle);

	cmd_list->SetDescriptorHeaps(1, dx12_->GetPeraCBVHeap().GetAddressOf());
	handle = dx12_->GetPeraCBVHeap()->GetGPUDescriptorHandleForHeapStart();
	cmd_list->SetGraphicsRootDescriptorTable(0, handle);

	cmd_list->SetDescriptorHeaps(1, dx12_->GetEffectSRVHeap().GetAddressOf());
	handle = dx12_->GetEffectSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	cmd_list->SetGraphicsRootDescriptorTable(2, handle);

	cmd_list->SetPipelineState(screen_pipeline_state_2_.Get());
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	D3D12_VERTEX_BUFFER_VIEW view = dx12_->GetScreenVertexBufferView();
	cmd_list->IASetVertexBuffers(0, 1, &view);
	cmd_list->DrawInstanced(4, 1, 0, 0);
}

void Renderer::EndDraw()
{
	auto bbIdx = dx12_->GetSwapChain()->GetCurrentBackBufferIndex();

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition
	(
		render_targets_[bbIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	dx12_->GetCommandList()->ResourceBarrier(1, &barrier);
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

void Renderer::PrepareZPrepass()
{
	auto cmd_list = dx12_->GetCommandList();

	auto handle = dx12_->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

	cmd_list->OMSetRenderTargets(0, nullptr, false, &handle);

	cmd_list->ClearDepthStencilView(handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	cmd_list->RSSetViewports(1, view_port_.get());
	cmd_list->RSSetScissorRects(1, scissor_rect_.get());
}

void Renderer::BeforeDrawFBXZPrepass()
{
	auto cmd_list = dx12_->GetCommandList();

	cmd_list->SetPipelineState(fbx_zprepass_pipeline_state_.Get());
	cmd_list->SetGraphicsRootSignature(fbx_model_root_signature_.Get());

	ID3D12DescriptorHeap* heaps[] = { dx12_->GetSceneCBVHeap().Get() };

	cmd_list->SetDescriptorHeaps(1, heaps);
	cmd_list->SetGraphicsRootDescriptorTable(0, dx12_->GetSceneCBVHeap()->GetGPUDescriptorHandleForHeapStart());
}

void Renderer::BeforeDrawPrimitiveZPrepass()
{
	auto cmd_list = dx12_->GetCommandList();

	cmd_list->SetPipelineState(primitive_zprepass_pipeline_state_.Get());
	cmd_list->SetGraphicsRootSignature(primitive_root_signature_.Get());

	ID3D12DescriptorHeap* heaps[] = { dx12_->GetSceneCBVHeap().Get() };

	cmd_list->SetDescriptorHeaps(1, heaps);
	cmd_list->SetGraphicsRootDescriptorTable(0, dx12_->GetSceneCBVHeap()->GetGPUDescriptorHandleForHeapStart());
}

void Renderer::PrepareGBuffer()
{
	// �o���A
	for (auto& g_buffer : g_buffers_)
	{
		BarrierTransResource(g_buffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	auto cmd_list = dx12_->GetCommandList();
	auto rtv_handle = off_screen_rtv_heap_->GetCPUDescriptorHandleForHeapStart();
	auto dsv_handle = dx12_->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart(); // ZPrepass�Őݒ肵�����̂����̂܂܎g��

	rtv_handle.ptr += static_cast<unsigned long long>(dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)) * 2; // �f�B�t�@�[�h���C�e�B���O�p + �|�X�g�G�t�F�N�g�p

	D3D12_CPU_DESCRIPTOR_HANDLE g_buffer_handles[kNumGBuffer]{};
	for (int i = 0; i < kNumGBuffer; ++i)
	{
		g_buffer_handles[i] = rtv_handle;
		rtv_handle.ptr += dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// �����_�[�^�[�Q�b�g�̐ݒ�
	cmd_list->OMSetRenderTargets(kNumGBuffer, g_buffer_handles, false, &dsv_handle);

	// �����_�[�^�[�Q�b�g�̃N���A
	float clear_color[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	for (int i = 0; i < kNumGBuffer; ++i)
	{
		cmd_list->ClearRenderTargetView(g_buffer_handles[i], clear_color, 0, nullptr);
	}

	cmd_list->RSSetViewports(1, view_port_.get());
	cmd_list->RSSetScissorRects(1, scissor_rect_.get());
}

void Renderer::BeforeDrawFBXGBuffer()
{
	auto cmd_list = dx12_->GetCommandList();

	cmd_list->SetPipelineState(fbx_gbuffer_pipeline_state_.Get());
	cmd_list->SetGraphicsRootSignature(fbx_model_root_signature_.Get());
}

void Renderer::BeforeDrawPrimitiveGBuffer()
{
	auto cmd_list = dx12_->GetCommandList();

	cmd_list->SetPipelineState(primitive_gbuffer_pipeline_state_.Get());
	cmd_list->SetGraphicsRootSignature(primitive_root_signature_.Get());
}

void Renderer::EndDrawGBuffer()
{
	// �o���A
	for (auto& g_buffer : g_buffers_)
	{
		BarrierTransResource(g_buffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
}

bool Renderer::CheckShaderCompileResult(HRESULT result, ID3DBlob* error)
{
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			OutputDebugStringA("�t�@�C����������܂���");
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
