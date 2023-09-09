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
	Renderer(class Dx12Wrapper& dx);
	~Renderer();
	void Initialize();
	void BeforePMDDraw();
	void BeforeFBXDraw();
	void BeforePrimitiveDraw();

	void BeforeDrawPMDShadowMap();
	void BeforeDrawFBXShadowMap();
	void BeforeDrawPrimitiveShadowMap();

	void Draw(bool is_shadow);

	// 描画
	void DrawPMDModel(bool is_shadow);
	void DrawFBXModel(bool is_shadow);
	void DrawPrimitive(bool is_shadow);

	void AddModelComponent(class ModelComponent* model, ModelType type);
	void RemoveModelComponent(class ModelComponent* model);

	void AddPrimitiveComponent(class PrimitiveComponent* primitive);
	void RemovePrimitiveComponent(class PrimitiveComponent* primitive);

	class Dx12Wrapper& GetDx() { return dx12_; }

	// 汎用テクスチャ
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
	class Dx12Wrapper& dx12_;
	// パイプライン
	ComPtr<ID3D12PipelineState> pmd_model_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> fbx_model_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> primitive_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> screen_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> screen_pipeline_state_2_ = nullptr;

	// 影用
	ComPtr<ID3D12PipelineState> pmd_shadow_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> fbx_shadow_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> primitive_shadow_pipeline_state_ = nullptr;

	// ルートシグネチャ
	ComPtr<ID3D12RootSignature> pmd_model_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> fbx_model_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> primitive_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> screen_root_signature_ = nullptr;

	// モデル
	std::vector<class ModelComponent*> pmd_models_;
	std::vector<class ModelComponent*> fbx_models_;
	std::vector<class PrimitiveComponent*> primitives_;

	ID3D12Resource* CreateDefaultTexture(size_t width, size_t height);

	ID3D12Resource* CreateWhiteTexture();
	ID3D12Resource* CreateBlackTexture();
	ID3D12Resource* CreateGradTexture();

	// パイプライン初期化
	HRESULT CreatePMDModelGraphicsPipeline();
	HRESULT CreateFBXModelGraphicsPipeline();
	HRESULT CreatePrimitiveGraphicsPipeline();
	HRESULT CreateScreenGraphicsPipeline();

	// ルートシグネチャ初期化
	HRESULT CreatePMDModelRootSignature();
	HRESULT CreateFBXModelRootSignature();
	HRESULT CreatePrimitiveRootSignature();
	HRESULT CreateScreenRootSignature();

	bool CheckShaderCompileResult(HRESULT result, ID3DBlob* error = nullptr);
};