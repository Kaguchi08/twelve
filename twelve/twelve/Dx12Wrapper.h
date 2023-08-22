#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <map>
#include <memory>
#include <unordered_map>
#include <DirectXTex.h>
#include <wrl.h>
#include <string>
#include <functional>
#include "Model.h"
#include "ResourceManager.h"


class PMDRenderer;
class PMDActor;
struct PMDModel;

using Microsoft::WRL::ComPtr;

class Dx12Wrapper
{
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	bool Initialize();

	bool PreDrawToPera();
	void DrawToPera1();
	void DrawToPera2();

	bool Clear();

	void DrawToBackBuffer();

	void SetCameraSetting();
	void SetScene();

	void EndDraw();
	void ExecuteCommand();

	void CreateImguiWindow();
	void RenderImgui();

	ComPtr<ID3D12Device> GetDevice()
	{
		return dev_.Get();
	}

	ComPtr<ID3D12GraphicsCommandList> GetCommandList()
	{
		return cmd_list_.Get();
	}

	ComPtr<IDXGISwapChain> GetSwapChain()
	{
		return swap_chain_.Get();
	}

	// リソースマネージャーを返す
	ResourceManager* GetResourceManager()
	{
		return resource_manager_.get();
	}

	const DirectX::XMFLOAT3& GetEye() const
	{
		return eye_;
	}
	const DirectX::XMFLOAT3& GetTarget() const
	{
		return target_;
	}
	const DirectX::XMFLOAT3& GetUp() const
	{
		return up_;
	}

	void SetViewMatrix(const DirectX::XMMATRIX& view)
	{
		view_matrix_ = view;
	}
	void SetEye(const DirectX::XMFLOAT3& eye)
	{
		eye_ = eye;
	}

private:
	SIZE window_size_;

	HWND hwnd_;

	// リソースマネージャー
	std::unique_ptr<class ResourceManager> resource_manager_ = nullptr;

	std::shared_ptr<class Renderer> renderer_ = nullptr;
	std::unique_ptr<class ModelLoader> model_loader_;

	// DXGI
	ComPtr<IDXGIFactory4> dxgi_factory_ = nullptr;
	ComPtr<IDXGISwapChain4> swap_chain_ = nullptr;

	// DirextX
	ComPtr<ID3D12Device> dev_ = nullptr;
	ComPtr<ID3D12CommandAllocator> mCmdAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList> cmd_list_ = nullptr;
	ComPtr<ID3D12CommandQueue> mCmdQueue = nullptr;

	// 表示に関わるバッファ周り
	ComPtr<ID3D12Resource> depth_buffer_ = nullptr;
	std::vector<ID3D12Resource*> back_buffers_;
	ComPtr<ID3D12DescriptorHeap> rtv_heap_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> dsv_heap_ = nullptr;
	std::unique_ptr<D3D12_VIEWPORT> view_port_;
	std::unique_ptr<D3D12_RECT> scissor_rect_;

	// シーンを構成するバッファ周り
	ComPtr<ID3D12Resource> scene_const_buff_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> scene_heap_ = nullptr;

	struct SceneMatrix
	{
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMMATRIX light_view;
		DirectX::XMMATRIX shadow;
		DirectX::XMFLOAT3 eye;
	};

	SceneMatrix* scene_matrix_ = nullptr;

	DirectX::XMFLOAT3 eye_;
	DirectX::XMFLOAT3 target_;
	DirectX::XMFLOAT3 up_;
	DirectX::XMMATRIX view_matrix_;

	// 平行ライト
	DirectX::XMFLOAT3 parallel_light_dir_;

	float fov_ = DirectX::XM_PI / 4;

	// フェンス
	ComPtr<ID3D12Fence> fence_ = nullptr;
	UINT64 fence_val_ = 0;

	void BarrierTransResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	// レンダーターゲットの生成
	HRESULT CreateRenderTarget();
	// 深度バッファの作成
	bool CreateDepthBuffer();
	// DSVの作成
	bool CreateDSV();
	// スワップチェーンの生成
	HRESULT CreateSwapChain();
	// DXGI周りの初期化
	HRESULT InitializeDXGIDevice();
	// コマンド周りの初期化
	HRESULT InitializeCommand();
	// ビュープロジェクション用ビューの生成
	HRESULT CreateSceneView();

	HRESULT InitializeDebug();
	HRESULT InitializeRenderer();
	HRESULT CreateFence();

	HRESULT CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT num_descriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ComPtr<ID3D12DescriptorHeap>& heap, UINT node_mask = 0);
	HRESULT CreateRenderTargetView(ID3D12Resource* resource, DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE handle);

	// テクスチャを張り付けるポリゴン
	ComPtr<ID3D12Resource> screen_resource_ = nullptr;
	ComPtr<ID3D12Resource> screen_resource_2_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> screen_rtv_heap_;
	ComPtr<ID3D12DescriptorHeap> sceen_srv_heap_;

	ComPtr<ID3D12Resource> screen_vertex_buffer_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW screen_vertex_buffer_view_;

	ComPtr<ID3D12PipelineState> screen_pipeline_;
	ComPtr<ID3D12PipelineState> screen_pipeline_2_;
	ComPtr<ID3D12RootSignature> screen_root_signature_;

	// ぼかし
	ComPtr<ID3D12Resource> pera_const_buffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> pera_cbv_heap_;

	// 歪み
	ComPtr<ID3D12Resource> effect_resource_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> effect_srv_heap_;

	// 深度値テクスチャ
	ComPtr<ID3D12DescriptorHeap> depth_srv_heap_ = nullptr;

	bool CreatePeraResourceAndView();
	bool CreatePeraConstBufferAndView();
	bool CreateEffectResourceAndView();
	bool CreatePeraVerTex();
	bool CreatePeraPipeline();
	bool CreateDepthSRV();

	// imgui
	ComPtr<ID3D12DescriptorHeap> imgui_heap_ = nullptr;

	bool InitializeImGui();
};