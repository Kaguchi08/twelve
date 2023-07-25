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


	// ���f���̓ǂݍ���
	std::shared_ptr<PMDModel> LoadPMDModel(const char* file_path);
	std::shared_ptr<FBXModel> LoadFBXModel(const char* file_path);

	// �e�N�X�`���p�X����K�v�ȃe�N�X�`���o�b�t�@�ւ̃|�C���^��Ԃ�
	ComPtr<ID3D12Resource> GetTextureFromPath(const char* tex_path);

	ComPtr<ID3D12Device> GetDevice() { return dev_.Get(); }

	ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return cmd_list_.Get(); }

	ComPtr<IDXGISwapChain> GetSwapChain() { return swap_chain_.Get(); }

	const DirectX::XMFLOAT3& GetEye() const { return eye_; }
	const DirectX::XMFLOAT3& GetTarget() const { return target_; }
	const DirectX::XMFLOAT3& GetUp() const { return up_; }

	void SetViewMatrix(const DirectX::XMMATRIX& view) { view_matrix_ = view; }
	void SetEye(const DirectX::XMFLOAT3& eye) { eye_ = eye; }

private:
	SIZE window_size_;

	HWND hwnd_;
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
	DirectX::XMMATRIX view_matrix_;

	// ���s���C�g
	DirectX::XMFLOAT3 parallel_light_dir_;

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

	// ���[�h�p�e�[�u��
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t> mLoadLambdaTable;

	// �e�N�X�`���e�[�u��
	std::unordered_map<std::string, ComPtr<ID3D12Resource>> mTextureTable;

	// �e�N�X�`�����[�_�e�[�u���̍쐬
	void CreateTextureLoaderTable();
	// �e�N�X�`��������e�N�X�`���o�b�t�@�쐬�A�R�s�[
	ID3D12Resource* CreateTextureFromFile(const char* tex_path);

	// �e�N�X�`���𒣂�t����|���S��
	ComPtr<ID3D12Resource> screen_resource_ = nullptr;
	ComPtr<ID3D12Resource> screen_resource_2_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> screen_rtv_heap_;
	ComPtr<ID3D12DescriptorHeap> sceen_srv_heap_;

	ComPtr<ID3D12Resource> screen_vertex_buffer_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW screen_vertex_buffer_view_;

	ComPtr<ID3D12PipelineState> screen_pipeline_;
	ComPtr<ID3D12PipelineState> screen_pipeline_2_;
	ComPtr<ID3D12RootSignature> screen_root_signature_;

	// �ڂ���
	ComPtr<ID3D12Resource> pera_const_buffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> pera_cbv_heap_;

	// �c��
	ComPtr<ID3D12Resource> effect_resource_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> effect_srv_heap_;

	// �[�x�l�e�N�X�`��
	ComPtr<ID3D12DescriptorHeap> depth_srv_heap_ = nullptr;

	// �ǂݍ��� Model �̃e�[�u��
	std::unordered_map<std::string, std::shared_ptr<PMDModel>> pmd_model_table_;
	std::unordered_map<std::string, std::shared_ptr<FBXModel>> fbx_model_table_;

	bool CreatePeraResourceAndView();
	bool CreatePeraConstBufferAndView();
	bool CreateEffectResourceAndView();
	bool CreatePeraVerTex();
	bool CreatePeraPipeline();
	bool CreateDepthSRV();

};