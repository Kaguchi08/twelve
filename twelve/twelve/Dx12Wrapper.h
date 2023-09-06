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

	bool Clear();

	void DrawToBackBuffer();

	void SetCameraSetting();
	void SetScene();

	void EndDraw();
	void ExecuteCommand();

	void CreateImguiWindow();
	void RenderImgui();

	ComPtr<ID3D12Device> GetDevice() { return dev_.Get(); }

	ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return cmd_list_.Get(); }

	ComPtr<IDXGISwapChain> GetSwapChain() { return swap_chain_.Get(); }

	// ���\�[�X�}�l�[�W���[��Ԃ�
	ResourceManager* GetResourceManager() { return resource_manager_.get(); }

	const DirectX::XMFLOAT3& GetEye() const { return eye_; }
	const DirectX::XMFLOAT3& GetTarget() const { return target_; }
	const DirectX::XMFLOAT3& GetUp() const { return up_; }

	void SetViewMatrix(const DirectX::XMMATRIX& view) { view_matrix_ = view; }
	void SetEye(const DirectX::XMFLOAT3& eye) { eye_ = eye; }

private:
	SIZE window_size_;

	HWND hwnd_;

	// ���\�[�X�}�l�[�W���[
	std::unique_ptr<class ResourceManager> resource_manager_ = nullptr;

	// ���C�g
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

	// �\���Ɋւ��o�b�t�@����
	ComPtr<ID3D12Resource> depth_buffer_ = nullptr;
	std::vector<ID3D12Resource*> back_buffers_;
	ComPtr<ID3D12DescriptorHeap> rtv_heap_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> dsv_heap_ = nullptr;
	std::unique_ptr<D3D12_VIEWPORT> view_port_;
	std::unique_ptr<D3D12_RECT> scissor_rect_;

	// �V�[�����\������o�b�t�@����
	ComPtr<ID3D12Resource> scene_const_buff_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> scene_csv_heap_ = nullptr;

	struct SceneMatrix
	{
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMMATRIX shadow;
		DirectX::XMFLOAT3 eye;
	};

	SceneMatrix* scene_matrix_ = nullptr;

	DirectX::XMFLOAT3 eye_;
	DirectX::XMFLOAT3 target_;
	DirectX::XMFLOAT3 up_;
	DirectX::XMMATRIX view_matrix_;

	float fov_ = DirectX::XM_PI / 4;

	// �t�F���X
	ComPtr<ID3D12Fence> fence_ = nullptr;
	UINT64 fence_val_ = 0;

	void BarrierTransResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	// �����_�[�^�[�Q�b�g�̐���
	HRESULT CreateRenderTarget();
	// �[�x�o�b�t�@�̍쐬
	bool CreateDepthBuffer();
	// DSV�̍쐬
	bool CreateDSV();
	// �X���b�v�`�F�[���̐���
	HRESULT CreateSwapChain();
	// DXGI����̏�����
	HRESULT InitializeDXGIDevice();
	// �R�}���h����̏�����
	HRESULT InitializeCommand();
	// �r���[�v���W�F�N�V�����p�r���[�̐���
	HRESULT CreateSceneView();

	// ���C�g
	HRESULT CreateLight();

	HRESULT InitializeDebug();
	HRESULT InitializeRenderer();
	HRESULT CreateFence();

	// �ꊇ�������܂Ƃ߂��֐�
	HRESULT CreateDescriptorHeapWrapper(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT num_descriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ComPtr<ID3D12DescriptorHeap>& heap, UINT node_mask = 0);
	HRESULT CreateRenderTargetViewWrapper(ID3D12Resource* resource, DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE handle);

	// �e�N�X�`���𒣂�t����|���S��
	ComPtr<ID3D12Resource> screen_resource_ = nullptr;
	ComPtr<ID3D12Resource> screen_resource_2_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> screen_rtv_heap_;
	ComPtr<ID3D12DescriptorHeap> sceen_srv_heap_;

	ComPtr<ID3D12Resource> screen_vertex_buffer_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW screen_vertex_buffer_view_;

	// �ڂ���
	ComPtr<ID3D12Resource> pera_const_buffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> pera_cbv_heap_;

	// �c��
	ComPtr<ID3D12Resource> effect_resource_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> effect_srv_heap_;

	// �[�x�l�e�N�X�`��
	ComPtr<ID3D12DescriptorHeap> depth_srv_heap_ = nullptr;

	bool CreatePeraResourceAndView();
	bool CreatePeraConstBufferAndView();
	bool CreateEffectResourceAndView();
	bool CreatePeraVerTex();
	bool CreateDepthSRV();

	// imgui
	ComPtr<ID3D12DescriptorHeap> imgui_heap_ = nullptr;

	bool InitializeImGui();
};
