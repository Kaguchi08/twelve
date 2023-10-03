#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include <array>
#include "Helper.h"
#include "Model.h"
#include "Primitive.h"
#include "Game.h"


using Microsoft::WRL::ComPtr;

const int kLightDepthMapSize = 4096;

class Renderer
{
public:
	Renderer(class Dx12Wrapper* dx);
	~Renderer();
	void Initialize();

	void Draw(GameState state);

	void AddModelComponent(class ModelComponent* model);
	void RemoveModelComponent(class ModelComponent* model);

	void AddFBXComponent(class FBXComponent* model);
	void RemoveFBXComponent(class FBXComponent* model);

	void AddPrimitiveComponent(class PrimitiveComponent* primitive);
	void RemovePrimitiveComponent(class PrimitiveComponent* primitive);

	class Dx12Wrapper* GetDx() { return dx12_; }

	// 汎用テクスチャ
	ComPtr<ID3D12Resource> white_texture_ = nullptr;
	ComPtr<ID3D12Resource> black_texture_ = nullptr;
	ComPtr<ID3D12Resource> grad_texture_ = nullptr;

private:
	// G-Buffer の定義
	enum GBuffer
	{
		kAlbedo,
		kNormal,
		kWorldPos,
		kArm,
		kShadow,
		kNumGBuffer,
	};

	class Dx12Wrapper* dx12_;

	// パイプライン
	ComPtr<ID3D12PipelineState> pmd_model_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> fbx_model_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> primitive_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> deferred_lighting_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> post_effect_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> main_pipeline_state_ = nullptr;

	// 影用
	ComPtr<ID3D12PipelineState> pmd_shadow_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> fbx_shadow_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> primitive_shadow_pipeline_state_ = nullptr;

	// ZPrepass
	ComPtr<ID3D12PipelineState> fbx_zprepass_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> primitive_zprepass_pipeline_state_ = nullptr;

	// G-Buffer
	ComPtr<ID3D12PipelineState> fbx_gbuffer_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> primitive_gbuffer_pipeline_state_ = nullptr;

	// ルートシグネチャ
	ComPtr<ID3D12RootSignature> pmd_model_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> fbx_model_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> primitive_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> deferred_lighting_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> post_effect_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> main_root_signature_ = nullptr;

	// モデル
	std::vector<class ModelComponent*> pmd_models_;
	std::vector<class FBXComponent*> fbx_models_;
	std::vector<class PrimitiveComponent*> primitives_;

	// レンダーターゲット
	std::vector<ID3D12Resource*> render_targets_;
	ComPtr<ID3D12DescriptorHeap> rtv_heap_ = nullptr;

	// オフスクリーンレンダリング
	ComPtr<ID3D12Resource> deferred_lighting_render_target = nullptr;
	ComPtr<ID3D12Resource> post_effect_render_target_ = nullptr;
	std::array<ComPtr<ID3D12Resource>, kNumGBuffer> g_buffers_;
	ComPtr<ID3D12DescriptorHeap> off_screen_rtv_heap_;
	ComPtr<ID3D12DescriptorHeap> off_sceen_srv_heap_;

	std::unique_ptr<D3D12_VIEWPORT> view_port_;
	std::unique_ptr<D3D12_RECT> scissor_rect_;

	ID3D12Resource* CreateDefaultTexture(size_t width, size_t height);

	ID3D12Resource* CreateWhiteTexture();
	ID3D12Resource* CreateBlackTexture();
	ID3D12Resource* CreateGradTexture();

	// パイプライン初期化
	HRESULT CreatePMDModelGraphicsPipeline();
	HRESULT CreateFBXModelGraphicsPipeline();
	HRESULT CreatePrimitiveGraphicsPipeline();
	HRESULT CreateDefferedLightingGraphicsPipeline();
	HRESULT CreatePostEffectGraphicsPipeline();
	HRESULT CreateMainGraphicsPipeline();

	// ルートシグネチャ初期化
	HRESULT CreatePMDModelRootSignature();
	HRESULT CreateFBXModelRootSignature();
	HRESULT CreatePrimitiveRootSignature();
	HRESULT CreateDeferredLightingRootSignature();
	HRESULT CreatePostEffectRootSignature();
	HRESULT CreateMainRootSignature();

	// レンダーターゲット
	HRESULT CreateRenderTarget();

	bool CreateOffScreenResourceAndView();

	void BarrierTransResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	// 描画
	// シャドウマップへの描画
	void DrawToShadowMap();

	// ZPrepass
	void DrawToZPrepass();

	// G-Buffer
	void DrawToGBuffer();

	// ディファードライティング
	void DrawDeferredLighting();

	// フォワードレンダリング
	void ForwardRendering();

	// ポストエフェクト
	void PostEffect();

	// フレームバッファへの描画
	void DrawToFrameBuffer();

	void EndDraw();

	// オブジェクト
	void DrawPMDModel(bool is_shadow);
	void DrawFBXModel(bool is_shadow);
	void DrawPrimitive(bool is_shadow);

	// シャドウマップ
	void PrepareShadowMap();
	void BeforeDrawPMDShadowMap();
	void BeforeDrawFBXShadowMap();
	void BeforeDrawPrimitiveShadowMap();

	// Zprepass
	void PrepareZPrepass();
	void BeforeDrawFBXZPrepass();
	void BeforeDrawPrimitiveZPrepass();

	// G-Buffer
	void PrepareGBuffer();
	void BeforeDrawFBXGBuffer();
	void BeforeDrawPrimitiveGBuffer();
	void EndDrawGBuffer();

	// フォワードレンダリング
	void PrepareForwardRendering();
	void BeforeDrawPMD();
	void EndForwardRendering();

	// Helperに移動してもいいかも
	bool CheckShaderCompileResult(HRESULT result, ID3DBlob* error = nullptr);
};