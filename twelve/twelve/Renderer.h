#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include "Helper.h"
#include "Model.h"


using Microsoft::WRL::ComPtr;

class Renderer
{
public:
	Renderer(class Dx12Wrapper& dx);
	~Renderer();
	void Initialize();
	void BeforeDraw();

	void Draw();

	void AddModelComponent(class ModelComponent* model, ModelType type);
	void RemoveModelComponent(class ModelComponent* model);

	class Dx12Wrapper& GetDx() { return dx12_; }

	// �ėp�e�N�X�`��
	ComPtr<ID3D12Resource> white_texture_ = nullptr;
	ComPtr<ID3D12Resource> black_texture_ = nullptr;
	ComPtr<ID3D12Resource> grad_texture_ = nullptr;

private:
	class Dx12Wrapper& dx12_;
	// �p�C�v���C��
	ComPtr<ID3D12PipelineState> pmd_model_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> fbx_model_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> screen_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> screen_pipeline_state_2_ = nullptr;

	// ���[�g�V�O�l�`��
	ComPtr<ID3D12RootSignature> pmd_model_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> fbx_model_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> screen_root_signature_ = nullptr;

	// ���f��
	std::vector<class ModelComponent*> pmd_models_;
	std::vector<class ModelComponent*> pending_pmd_models_;

	std::vector<class ModelComponent*> fbx_models_;
	std::vector<class ModelComponent*> pending_fbx_models_;

	ID3D12Resource* CreateDefaultTexture(size_t width, size_t height);

	ID3D12Resource* CreateWhiteTexture();
	ID3D12Resource* CreateBlackTexture();
	ID3D12Resource* CreateGradTexture();

	// �p�C�v���C��������
	HRESULT CreatePMDModelGraphicsPipeline();
	HRESULT CreateFBXModelGraphicsPipeline();
	HRESULT CreateScreenGraphicsPipeline();

	// ���[�g�V�O�l�`��������
	HRESULT CreatePMDModelRootSignature();
	HRESULT CreateFBXModelRootSignature();
	HRESULT CreateScreenRootSignature();

	// �`��
	void DrawPMDModel();
	void DrawFBXModel();

	bool CheckShaderCompileResult(HRESULT result, ID3DBlob* error = nullptr);
};