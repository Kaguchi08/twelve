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

class PMDRenderer;
class PMDActor;

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

	void Draw();

	void SetCameraSetting();

	void EndDraw();
	void ExecuteCommand();

	// �e�N�X�`���p�X����K�v�ȃe�N�X�`���o�b�t�@�ւ̃|�C���^��Ԃ�
	ComPtr<ID3D12Resource> GetTextureFromPath(const char* texPath);

	ComPtr<ID3D12Device> GetDevice() {
		return dev_.Get();
	}

	ComPtr<ID3D12GraphicsCommandList> GetCommandList() {
		return cmd_list_.Get();
	}

	ComPtr<IDXGISwapChain> GetSwapChain() {
		return swap_chain_.Get();
	}

	void SetScene();

private:
	SIZE window_size_;

	HWND hwnd_;

	// DXGI
	ComPtr<IDXGIFactory4> dxgi_factory_ = nullptr;
	ComPtr<IDXGISwapChain4> swap_chain_ = nullptr;

	// DirextX
	ComPtr<ID3D12Device> dev_ = nullptr;
	ComPtr<ID3D12CommandAllocator> mCmdAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList> cmd_list_ = nullptr;
	ComPtr<ID3D12CommandQueue> mCmdQueue = nullptr;

	// �\���Ɋւ��o�b�t�@����
	ComPtr<ID3D12Resource> depth_buffer_ = nullptr;
	std::vector<ID3D12Resource*> back_buffers_;
	ComPtr<ID3D12DescriptorHeap> rtv_heap_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> dsv_heap_ = nullptr;
	std::unique_ptr<D3D12_VIEWPORT> view_port_;
	std::unique_ptr<D3D12_RECT> scissor_rect_;

	// �V�[�����\������o�b�t�@����
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

	// ���s���C�g
	DirectX::XMFLOAT3 parallel_light_dir_;

	float fov_ = DirectX::XM_PI / 6;

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

	// ���[�h�p�e�[�u��
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t> mLoadLambdaTable;

	// �e�N�X�`���e�[�u��
	std::unordered_map<std::string, ComPtr<ID3D12Resource>> mTextureTable;

	// �e�N�X�`�����[�_�e�[�u���̍쐬
	void CreateTextureLoaderTable();
	// �e�N�X�`��������e�N�X�`���o�b�t�@�쐬�A�R�s�[
	ID3D12Resource* CreateTextureFromFile(const char* texPath);

	// �y���|���S��
	ComPtr<ID3D12Resource> pera_resource_ = nullptr;
	ComPtr<ID3D12Resource> pera_resource_2_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> pera_rtv_heap_;
	ComPtr<ID3D12DescriptorHeap> pera_srv_heap_;

	ComPtr<ID3D12Resource> pera_vertex_buffer_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW pera_vertex_buffer_view_;

	ComPtr<ID3D12PipelineState> pera_pipeline_;
	ComPtr<ID3D12PipelineState> pera_pipeline_2_;
	ComPtr<ID3D12RootSignature> pera_root_signature_;

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
	bool CreatePeraPipeline();
	bool CreateDepthSRV();

};