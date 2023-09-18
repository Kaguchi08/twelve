#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include "Helper.h"
#include "Model.h"
#include "Primitive.h"


using Microsoft::WRL::ComPtr;

class Renderer
{
public:
	Renderer(class Dx12Wrapper* dx);
	~Renderer();
	void Initialize();

	// �V���h�E�}�b�v�ւ̕`��
	void DrawToShadowMap();

	void BeforePMDDraw();
	void BeforeFBXDraw();
	void BeforePrimitiveDraw();

	void Draw(bool is_shadow);

	// �`��
	void DrawPMDModel(bool is_shadow);
	void DrawFBXModel(bool is_shadow);
	void DrawPrimitive(bool is_shadow);

	void AddModelComponent(class ModelComponent* model);
	void RemoveModelComponent(class ModelComponent* model);

	void AddFBXComponent(class FBXComponent* model);
	void RemoveFBXComponent(class FBXComponent* model);

	void AddPrimitiveComponent(class PrimitiveComponent* primitive);
	void RemovePrimitiveComponent(class PrimitiveComponent* primitive);

	class Dx12Wrapper* GetDx() { return dx12_; }

	// �ėp�e�N�X�`��
	ComPtr<ID3D12Resource> white_texture_ = nullptr;
	ComPtr<ID3D12Resource> black_texture_ = nullptr;
	ComPtr<ID3D12Resource> grad_texture_ = nullptr;

	// geter/setter
	ComPtr<ID3D12PipelineState> GetPMDModelPipelineState() { return pmd_model_pipeline_state_; }
	ComPtr<ID3D12PipelineState> GetFBXModelPipelineState() { return fbx_model_pipeline_state_; }
	ComPtr<ID3D12PipelineState> GetScreenPipelineState() { return screen_pipeline_state_; }
	ComPtr<ID3D12PipelineState> GetScreenPipelineState2() { return screen_pipeline_state_2_; }

	ComPtr<ID3D12RootSignature> GetPMDModelRootSignature() { return pmd_model_root_signature_; }
	ComPtr<ID3D12RootSignature> GetFBXModelRootSignature() { return fbx_model_root_signature_; }
	ComPtr<ID3D12RootSignature> GetScreenRootSignature() { return screen_root_signature_; }


private:
	class Dx12Wrapper* dx12_;
	// �p�C�v���C��
	ComPtr<ID3D12PipelineState> pmd_model_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> fbx_model_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> primitive_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> screen_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> screen_pipeline_state_2_ = nullptr;

	// �e�p
	ComPtr<ID3D12PipelineState> pmd_shadow_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> fbx_shadow_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> primitive_shadow_pipeline_state_ = nullptr;

	// ���[�g�V�O�l�`��
	ComPtr<ID3D12RootSignature> pmd_model_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> fbx_model_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> primitive_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> screen_root_signature_ = nullptr;

	// ���f��
	std::vector<class ModelComponent*> pmd_models_;
	std::vector<class FBXComponent*> fbx_models_;
	std::vector<class PrimitiveComponent*> primitives_;

	ID3D12Resource* CreateDefaultTexture(size_t width, size_t height);

	ID3D12Resource* CreateWhiteTexture();
	ID3D12Resource* CreateBlackTexture();
	ID3D12Resource* CreateGradTexture();

	// �p�C�v���C��������
	HRESULT CreatePMDModelGraphicsPipeline();
	HRESULT CreateFBXModelGraphicsPipeline();
	HRESULT CreatePrimitiveGraphicsPipeline();
	HRESULT CreateScreenGraphicsPipeline();

	// ���[�g�V�O�l�`��������
	HRESULT CreatePMDModelRootSignature();
	HRESULT CreateFBXModelRootSignature();
	HRESULT CreatePrimitiveRootSignature();
	HRESULT CreateScreenRootSignature();

	// �V���h�E�}�b�v
	void PrepareShadowMap();
	void BeforeDrawPMDShadowMap();
	void BeforeDrawFBXShadowMap();
	void BeforeDrawPrimitiveShadowMap();

	// Helper�Ɉړ����Ă���������
	bool CheckShaderCompileResult(HRESULT result, ID3DBlob* error = nullptr);
};