#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include "Helper.h"

using Microsoft::WRL::ComPtr;

class Renderer {
public:
	Renderer(class Dx12Wrapper& dx);
	~Renderer();
	void Initialize();
	void BeforeDraw();

	void DrawToBackBuffer();

	class Dx12Wrapper& GetDx() { return dx12_; }

	// �ėp�e�N�X�`��
	ComPtr<ID3D12Resource> white_texture_ = nullptr;
	ComPtr<ID3D12Resource> black_texture_ = nullptr;
	ComPtr<ID3D12Resource> grad_texture_ = nullptr;

private:
	class Dx12Wrapper& dx12_;
	// �p�C�v���C��
	ComPtr<ID3D12PipelineState> model_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> screen_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> screen_pipeline_state_2_ = nullptr;

	// ���[�g�V�O�l�`��
	ComPtr<ID3D12RootSignature> model_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> screen_root_signature_ = nullptr;

	

	ID3D12Resource* CreateDefaultTexture(size_t width, size_t height);

	ID3D12Resource* CreateWhiteTexture();
	ID3D12Resource* CreateBlackTexture();
	ID3D12Resource* CreateGradTexture();

	// �p�C�v���C��������
	HRESULT CreateModelGraphicsPipeline();
	HRESULT CreateScreenGraphicsPipeline();

	// ���[�g�V�O�l�`��������
	HRESULT CreateModelRootSignature();
	HRESULT CreateScreenRootSignature();

	bool CheckShaderCompileResult(HRESULT result, ID3DBlob* error = nullptr);
};