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
#include "Light.h"


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

	void SetSceneCB();

	bool Clear();

	void DrawToBackBuffer();

	void SetCameraSetting();
	// バッファをセットする
	void SetCommonBuffer(UINT scene_index, UINT light_index, UINT depth_index);

	void EndDraw();
	void ExecuteCommand();

	void CreateImguiWindow();
	void RenderImgui();

	SIZE GetWindowSize() const { return window_size_; }

	ComPtr<ID3D12Device> GetDevice() { return dev_.Get(); }

	ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return cmd_list_.Get(); }

	ComPtr<IDXGISwapChain> GetSwapChain() { return swap_chain_.Get(); }

	ComPtr<ID3D12DescriptorHeap> GetRTVHeap() { return rtv_heap_; }
	ComPtr<ID3D12DescriptorHeap> GetDSVHeap() { return dsv_heap_; }
	ComPtr<ID3D12DescriptorHeap> GetSceneCBVHeap() { return scene_cbv_heap_; }

	std::unique_ptr<D3D12_VIEWPORT>& GetViewPort() { return view_port_; }
	std::unique_ptr<D3D12_RECT>& GetScissorRect() { return scissor_rect_; }

	// リソースマネージャーを返す
	ResourceManager* GetResourceManager() { return resource_manager_.get(); }

	const DirectX::XMFLOAT3& GetEye() const { return eye_; }
	const DirectX::XMFLOAT3& GetTarget() const { return target_; }
	const DirectX::XMFLOAT3& GetUp() const { return up_; }

	void SetViewMatrix(const DirectX::XMMATRIX& view) { view_matrix_ = view; }
	void SetEye(const DirectX::XMFLOAT3& eye) { eye_ = eye; }
	void SetTarget(const DirectX::XMFLOAT3& target) { target_ = target; }

private:
	SIZE window_size_;

	HWND hwnd_;

	// リソースマネージャー
	std::unique_ptr<class ResourceManager> resource_manager_ = nullptr;

	// ライト
	std::unique_ptr<class Light> light_ = nullptr;
	ComPtr<ID3D12Resource> light_const_buffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> light_csv_heap_ = nullptr;

	std::shared_ptr<class Renderer> renderer_ = nullptr;
	std::unique_ptr<class ModelLoader> model_loader_;

	// DXGI
	ComPtr<IDXGIFactory4> dxgi_factory_ = nullptr;
	ComPtr<IDXGISwapChain4> swap_chain_ = nullptr;

	// DirextX
	ComPtr<ID3D12Device> dev_ = nullptr;
	ComPtr<ID3D12CommandAllocator> cmd_allocator_ = nullptr;
	ComPtr<ID3D12GraphicsCommandList> cmd_list_ = nullptr;
	ComPtr<ID3D12CommandQueue> cmd_queue_ = nullptr;

	// 表示に関わるバッファ周り
	std::vector<ID3D12Resource*> render_targets_;
	ComPtr<ID3D12DescriptorHeap> rtv_heap_ = nullptr;
	std::unique_ptr<D3D12_VIEWPORT> view_port_;
	std::unique_ptr<D3D12_RECT> scissor_rect_;

	// 深度値
	ComPtr<ID3D12DescriptorHeap> dsv_heap_ = nullptr;
	ComPtr<ID3D12Resource> depth_buffer_ = nullptr; // Zprepassでも使用
	ComPtr<ID3D12Resource> light_depth_buffer_ = nullptr;

	// 深度値テクスチャ
	ComPtr<ID3D12DescriptorHeap> depth_srv_heap_ = nullptr;

	// シーンを構成するバッファ周り
	ComPtr<ID3D12Resource> scene_const_buff_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> scene_cbv_heap_ = nullptr;

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

	// ライト
	HRESULT CreateLight();

	HRESULT InitializeDebug();
	HRESULT InitializeRenderer();
	HRESULT CreateFence();

	// 一括処理をまとめた関数
	HRESULT CreateDescriptorHeapWrapper(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT num_descriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ComPtr<ID3D12DescriptorHeap>& heap, UINT node_mask = 0);
	HRESULT CreateRenderTargetViewWrapper(ID3D12Resource* resource, DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE handle);

	// オフスクリーンレンダリング
	// todo: 配列にする
	ComPtr<ID3D12Resource> screen_resource_ = nullptr;
	ComPtr<ID3D12Resource> screen_resource_2_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> screen_rtv_heap_;
	ComPtr<ID3D12DescriptorHeap> sceen_srv_heap_;

	ComPtr<ID3D12Resource> screen_vertex_buffer_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW screen_vertex_buffer_view_;

	// ぼかし
	ComPtr<ID3D12Resource> pera_const_buffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> pera_cbv_heap_;

	// 歪み
	ComPtr<ID3D12Resource> effect_resource_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> effect_srv_heap_;

	bool CreateScreenResourceAndView();
	bool CreatePeraConstBufferAndView();
	bool CreateEffectResourceAndView();
	bool CreatePeraVerTex();
	bool CreateDepthSRV();

	// imgui
	ComPtr<ID3D12DescriptorHeap> imgui_heap_ = nullptr;

	bool InitializeImGui();
};
