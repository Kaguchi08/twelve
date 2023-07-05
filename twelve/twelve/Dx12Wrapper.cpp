#include "Dx12Wrapper.h"
#include "Helper.h"
#include <cassert>
#include <d3dx12.h>
#include "PMDRenderer.h"
#include "PMDActor.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "Game.h"

using namespace Microsoft::WRL;
// "-" �I�y�����h���g���Ȃ�����
using namespace DirectX;

Dx12Wrapper::Dx12Wrapper(HWND hwnd):
	hwnd_(hwnd),
	eye_(0, 15, -50),
	target_(0, 12, 0),
	up_(0, 1, 0),
	parallel_light_dir_(1, -1, 1)
{
}

Dx12Wrapper::~Dx12Wrapper()
{
}

bool Dx12Wrapper::Initialize() {
	Game game;
	window_size_ = game.GetWindowSize();

	ID3D12Debug* debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
		debug->EnableDebugLayer();
		debug->Release();
	}

	if (FAILED(InitializeDXGIDevice())) {
		assert(0);
		return false;
	}

	if (FAILED(InitializeCommand())) {
		assert(0);
		return false;
	}

	if (FAILED(CreateSwapChain())) {
		assert(0);
		return false;
	}

	if (FAILED(CreateRenderTarget())) {
		assert(0);
		return false;
	}

	// �e�N�X�`�����[�_�֘A�̏�����
	CreateTextureLoaderTable();

	// �[�x
	if (!CreateDepthBuffer()) {
		return false;
	}

	if (!CreateDSV()) {
		return false;
	}

	if (!CreateDepthSRV()) {
		return false;
	}

	// �t�F���X�̍쐬
	if (FAILED(dev_->CreateFence(fence_val_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf())))) {
		assert(0);
		return false;
	}

	if (FAILED(CreateSceneView())) {
		assert(0);
		return false;
	}

	if (!CreateEffectResourceAndView()) {
		return false;
	}

	// �y���|��
	if (!CreatePeraResourceAndView()) {
		return false;
	}

	if (!CreatePeraConstBufferAndView()) {
		return false;
	}

	if (!CreatePeraVerTex()) {
		return false;
	}

	if (!CreatePeraPipeline()) {
		return false;
	}

	return true;
}

bool Dx12Wrapper::PreDrawToPera()
{
	BarrierTransResource(screen_resource_.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto rtvH = screen_rtv_heap_->GetCPUDescriptorHandleForHeapStart();
	auto dsvH = dsv_heap_->GetCPUDescriptorHandleForHeapStart();
	cmd_list_->OMSetRenderTargets(1, &rtvH, false, &dsvH);

	float clear_color[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	cmd_list_->ClearRenderTargetView(rtvH, clear_color, 0, nullptr);

	cmd_list_->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	/*SetScene();*/

	return true;
}

void Dx12Wrapper::DrawToPera1()
{
	Game game;
	auto window_size = game.GetWindowSize();

	cmd_list_->RSSetViewports(1, view_port_.get());
	cmd_list_->RSSetScissorRects(1, scissor_rect_.get());

	SetScene();

	//cmd_list_->SetDescriptorHeaps(1, depth_srv_heap_.GetAddressOf());

	//auto handle = depth_srv_heap_->GetGPUDescriptorHandleForHeapStart();
}

void Dx12Wrapper::DrawToPera2()
{
	BarrierTransResource(screen_resource_2_.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto rtvH = screen_rtv_heap_->GetCPUDescriptorHandleForHeapStart();

	rtvH.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	cmd_list_->OMSetRenderTargets(1, &rtvH, false, nullptr);

	cmd_list_->RSSetViewports(1, view_port_.get());
	cmd_list_->RSSetScissorRects(1, scissor_rect_.get());

	cmd_list_->SetGraphicsRootSignature(screen_root_signature_.Get());
	cmd_list_->SetDescriptorHeaps(1, sceen_srv_heap_.GetAddressOf());

	auto handle = sceen_srv_heap_->GetGPUDescriptorHandleForHeapStart();
	cmd_list_->SetGraphicsRootDescriptorTable(1, handle);

	cmd_list_->SetDescriptorHeaps(1, pera_cbv_heap_.GetAddressOf());
	handle = pera_cbv_heap_->GetGPUDescriptorHandleForHeapStart();
	cmd_list_->SetGraphicsRootDescriptorTable(0, handle);

	cmd_list_->SetPipelineState(screen_pipeline_.Get());
	cmd_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmd_list_->IASetVertexBuffers(0, 1, &screen_vertex_buffer_view_);
	cmd_list_->DrawInstanced(4, 1, 0, 0);

	BarrierTransResource(screen_resource_2_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

bool Dx12Wrapper::Clear()
{
	BarrierTransResource(screen_resource_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	auto bbIdx = swap_chain_->GetCurrentBackBufferIndex();

	BarrierTransResource(back_buffers_[bbIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto rtvH = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	cmd_list_->OMSetRenderTargets(1, &rtvH, false, nullptr);

	float clear_color[] = { 0.2f, 0.5f, 0.5f, 1.0f };
	cmd_list_->ClearRenderTargetView(rtvH, clear_color, 0, nullptr);

	return true;
}

void Dx12Wrapper::Draw()
{
	cmd_list_->RSSetViewports(1, view_port_.get());
	cmd_list_->RSSetScissorRects(1, scissor_rect_.get());

	cmd_list_->SetGraphicsRootSignature(screen_root_signature_.Get());
	cmd_list_->SetDescriptorHeaps(1, sceen_srv_heap_.GetAddressOf());

	auto handle = sceen_srv_heap_->GetGPUDescriptorHandleForHeapStart();
	cmd_list_->SetGraphicsRootDescriptorTable(0, handle);
	handle.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cmd_list_->SetGraphicsRootDescriptorTable(1, handle);

	cmd_list_->SetDescriptorHeaps(1, pera_cbv_heap_.GetAddressOf());
	handle = pera_cbv_heap_->GetGPUDescriptorHandleForHeapStart();
	cmd_list_->SetGraphicsRootDescriptorTable(0, handle);

	cmd_list_->SetDescriptorHeaps(1, effect_srv_heap_.GetAddressOf());
	handle = effect_srv_heap_->GetGPUDescriptorHandleForHeapStart();
	cmd_list_->SetGraphicsRootDescriptorTable(2, handle);

	cmd_list_->SetPipelineState(screen_pipeline_2_.Get());
	cmd_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmd_list_->IASetVertexBuffers(0, 1, &screen_vertex_buffer_view_);
	cmd_list_->DrawInstanced(4, 1, 0, 0);
}

void Dx12Wrapper::SetCameraSetting()
{
	Game game;
	auto w_size = game.GetWindowSize();

	auto eye_pos = DirectX::XMLoadFloat3(&eye_);
	auto target_pos = DirectX::XMLoadFloat3(&target_);
	auto up_vec = DirectX::XMLoadFloat3(&up_);
	
	scene_matrix_->eye = eye_;
	scene_matrix_->view = DirectX::XMMatrixLookAtLH(eye_pos, target_pos, up_vec);
	scene_matrix_->proj = DirectX::XMMatrixPerspectiveFovLH(
		fov_,
		static_cast<float>(w_size.cx) / static_cast<float>(w_size.cy),
		0.1f,
		1000.0f
	);

	DirectX::XMFLOAT4 plane_normal_vec = { 0.0f, 1.0f, 0.0f, 0.0f };

	scene_matrix_->shadow = DirectX::XMMatrixShadow(
		DirectX::XMLoadFloat4(&plane_normal_vec),
		-DirectX::XMLoadFloat3(&parallel_light_dir_)
	);


}

void Dx12Wrapper::EndDraw()
{
	auto bbIdx = swap_chain_->GetCurrentBackBufferIndex();

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition
	(
		back_buffers_[bbIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	cmd_list_->ResourceBarrier(1, &barrier);

	ExecuteCommand();
}

void Dx12Wrapper::ExecuteCommand()
{
	cmd_list_->Close();

	ID3D12CommandList* cmdLists[] = { cmd_list_.Get() };
	mCmdQueue->ExecuteCommandLists(1, cmdLists);

	mCmdQueue->Signal(fence_.Get(), ++fence_val_);

	if (fence_->GetCompletedValue() < fence_val_)
	{
		auto event = CreateEvent(nullptr, false, false, nullptr);
		fence_->SetEventOnCompletion(fence_val_, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}

	mCmdAllocator->Reset();
	cmd_list_->Reset(mCmdAllocator.Get(), nullptr);
}

ComPtr<ID3D12Resource> Dx12Wrapper::GetTextureFromPath(const char* texPath)
{
	auto it = mTextureTable.find(texPath);
	if (it != mTextureTable.end())
	{
		return it->second;
	}
	else
	{
		return ComPtr<ID3D12Resource>(CreateTextureFromFile(texPath));
	}
}

void Dx12Wrapper::SetScene()
{
	ID3D12DescriptorHeap* sceneHeaps[] = { scene_heap_.Get() };

	cmd_list_->SetDescriptorHeaps(1, sceneHeaps);
	cmd_list_->SetGraphicsRootDescriptorTable(0, scene_heap_->GetGPUDescriptorHandleForHeapStart());
}

void Dx12Wrapper::BarrierTransResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
	cmd_list_->ResourceBarrier(1, &barrier);
}

HRESULT Dx12Wrapper::CreateRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	auto result = swap_chain_->GetDesc1(&swap_chain_desc);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = swap_chain_desc.BufferCount;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = dev_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	back_buffers_.resize(swap_chain_desc.BufferCount);

	auto handle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
	auto inc_size = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// SRGB�����_�[�^�[�Q�b�g�r���[�̐ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (size_t i = 0; i < swap_chain_desc.BufferCount; ++i)
	{
		result = swap_chain_->GetBuffer(i, IID_PPV_ARGS(&back_buffers_[i]));
		assert(SUCCEEDED(result));
		if (FAILED(result))
		{
			assert(0);
			return result;
		}

		rtvDesc.Format = back_buffers_[i]->GetDesc().Format;
		dev_->CreateRenderTargetView(back_buffers_[i], &rtvDesc, handle);
		handle.ptr += inc_size;
	}


	// �r���[�|�[�g
	view_port_.reset(new CD3DX12_VIEWPORT(0.0f, 0.0f, window_size_.cx, window_size_.cy));
	scissor_rect_.reset(new CD3DX12_RECT(0.0f, 0.0f, window_size_.cx, window_size_.cy));

	return result;
}

bool Dx12Wrapper::CreateDepthBuffer()
{
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	auto result = swap_chain_->GetDesc1(&swap_chain_desc);

	D3D12_RESOURCE_DESC res_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, swap_chain_desc.Width, swap_chain_desc.Height);
	res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_CLEAR_VALUE clear_val(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	result = dev_->CreateCommittedResource
	(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clear_val,
		IID_PPV_ARGS(depth_buffer_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	return true;
}

bool Dx12Wrapper::CreateDSV()
{
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NodeMask = 0;
	heap_desc.NumDescriptors = 1;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	auto result = dev_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(dsv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// �[�x�X�e���V���r���[�̐ݒ�
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
	dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Flags = D3D12_DSV_FLAG_NONE;

	dev_->CreateDepthStencilView(depth_buffer_.Get(), &dsv_desc, dsv_heap_->GetCPUDescriptorHandleForHeapStart());

	return true;
}

HRESULT Dx12Wrapper::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_size_.cx;
	swapchainDesc.Height = window_size_.cy;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	auto result = dxgi_factory_->CreateSwapChainForHwnd
	(
		mCmdQueue.Get(),
		hwnd_,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)swap_chain_.ReleaseAndGetAddressOf()
	);

	assert(SUCCEEDED(result));
	
	return result;
}

HRESULT Dx12Wrapper::InitializeDXGIDevice()
{
	HRESULT result = S_OK;

	result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(dxgi_factory_.ReleaseAndGetAddressOf()));

	//DirectX12�܂�菉����
	//�t�B�[�`�����x����
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};


	//Direct3D�f�o�C�X�̏�����
	std::vector <IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; dxgi_factory_->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	result = S_FALSE;

	for (auto l : levels)
	{
		if (SUCCEEDED(D3D12CreateDevice(tmpAdapter, l, IID_PPV_ARGS(dev_.ReleaseAndGetAddressOf())))) {
			result = S_OK;
			break;
		}
	}

	if (FAILED(result)) {
		return result;
	}

	return result;
}

HRESULT Dx12Wrapper::InitializeCommand()
{
	auto result = dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCmdAllocator.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	result = dev_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocator.Get(), nullptr, IID_PPV_ARGS(cmd_list_.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	result = dev_->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(mCmdQueue.ReleaseAndGetAddressOf()));
	
	assert(SUCCEEDED(result));

	return result;
}

HRESULT Dx12Wrapper::CreateSceneView()
{
    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	auto result = swap_chain_->GetDesc1(&swapchainDesc);

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff);

	// �萔�o�b�t�@�̍쐬
	result = dev_->CreateCommittedResource
	(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(scene_const_buff_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}


	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.NodeMask = 0;
	heap_desc.NumDescriptors = 1;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = dev_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(scene_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	auto handle = scene_heap_->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC view_desc = {};
	view_desc.BufferLocation = scene_const_buff_->GetGPUVirtualAddress();
	view_desc.SizeInBytes = scene_const_buff_->GetDesc().Width;

	dev_->CreateConstantBufferView(&view_desc, handle);

	scene_matrix_ = nullptr;
	result = scene_const_buff_->Map(0, nullptr, (void**)&scene_matrix_);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	SetCameraSetting();

	return result;
}

void Dx12Wrapper::CreateTextureLoaderTable()
{
	mLoadLambdaTable["sph"]
		= mLoadLambdaTable["spa"]
		= mLoadLambdaTable["bmp"]
		= mLoadLambdaTable["png"]
		= mLoadLambdaTable["jpg"]
		= [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)
		-> HRESULT
	{
		return DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, meta, img);
	};

	mLoadLambdaTable["tga"]
		= [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)
		-> HRESULT
	{
		return DirectX::LoadFromTGAFile(path.c_str(), meta, img);
	};

	mLoadLambdaTable["dds"]
		= [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)
		-> HRESULT
	{
		return DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, meta, img);
	};
}

ID3D12Resource* Dx12Wrapper::CreateTextureFromFile(const char* texPath)
{
	std::string strTexPath = texPath;
	// WIC�e�N�X�`���̃��[�h
	DirectX::TexMetadata metadata = {};
	DirectX::ScratchImage scratchImg = {};

	auto wTexPath = GetWideStringFromString(strTexPath);
	auto ext = GetExtension(strTexPath);
	auto result = mLoadLambdaTable[ext](wTexPath, &metadata, scratchImg);

	if (FAILED(result))
	{
		return nullptr;
	}

	auto img = scratchImg.GetImage(0, 0, 0);

	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);

	ID3D12Resource* texBuff = nullptr;
	result = dev_->CreateCommittedResource
	(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texBuff)
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	// �e�N�X�`���p�o�b�t�@�ւ̏�������
	result = texBuff->WriteToSubresource
	(
		0,
		nullptr,
		img->pixels,
		img->rowPitch,
		img->slicePitch
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	return texBuff;
}

bool Dx12Wrapper::CreatePeraResourceAndView()
{
	// �g���Ă���o�b�N�o�b�t�@�[�̏��𗘗p
	auto& b_buff = back_buffers_[0];
	auto res_desc = b_buff->GetDesc();

	D3D12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	float clear_color[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	D3D12_CLEAR_VALUE clear_value = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clear_color);

	auto result = dev_->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clear_value,
		IID_PPV_ARGS(screen_resource_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	result = dev_->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clear_value,
		IID_PPV_ARGS(screen_resource_2_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	// RTV�쐬
	auto heap_desc = rtv_heap_->GetDesc();
	heap_desc.NumDescriptors = 2;

	result = dev_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(screen_rtv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	auto handle = screen_rtv_heap_->GetCPUDescriptorHandleForHeapStart();

	dev_->CreateRenderTargetView(screen_resource_.Get(), &rtv_desc, handle);

	handle.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	dev_->CreateRenderTargetView(screen_resource_2_.Get(), &rtv_desc, handle);

	// SRV�쐬
	heap_desc.NumDescriptors = 2;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.NodeMask = 0;

	result = dev_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(sceen_srv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Format = rtv_desc.Format;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	handle = sceen_srv_heap_->GetCPUDescriptorHandleForHeapStart();

	dev_->CreateShaderResourceView(screen_resource_.Get(), &srv_desc, handle);

	handle.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	dev_->CreateShaderResourceView(screen_resource_2_.Get(), &srv_desc, handle);

	return true;
}

bool Dx12Wrapper::CreatePeraConstBufferAndView()
{
	auto weights = GetGaussianWeights(8, 5.0f);
	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(AligmentedValue(sizeof(weights[0]) * weights.size(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

	auto result = dev_->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pera_const_buffer_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.NodeMask = 0;
	heap_desc.NumDescriptors = 1;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = dev_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(pera_cbv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = pera_const_buffer_->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = pera_const_buffer_->GetDesc().Width;

	auto handle = pera_cbv_heap_->GetCPUDescriptorHandleForHeapStart();

	dev_->CreateConstantBufferView(&cbv_desc, handle);

	float* mapped = nullptr;
	result = pera_const_buffer_->Map(0, nullptr, (void**)&mapped);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	std::copy(weights.begin(), weights.end(), mapped);
	pera_const_buffer_->Unmap(0, nullptr);
}

bool Dx12Wrapper::CreateEffectResourceAndView()
{
	//effect_resource_ = CreateTextureFromFile("../normal/crack_n.png");
	effect_resource_ = CreateTextureFromFile("../normal/normalmap.jpg");


	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.NumDescriptors = 1;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = dev_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(effect_srv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	auto desc = effect_resource_->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Format = desc.Format;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto handle = effect_srv_heap_->GetCPUDescriptorHandleForHeapStart();

	dev_->CreateShaderResourceView(effect_resource_.Get(), &srv_desc, handle);

	return true;
}

bool Dx12Wrapper::CreatePeraVerTex() {
	struct PeraVertex {
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};

	PeraVertex pv[4] = {
		{{-1, -1, 0.1}, {0, 1}},
		{{-1, 1, 0.1}, {0, 0}},
		{{1, -1, 0.1}, {1, 1}},
		{{1, 1, 0.1}, {1, 0}}
	};

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(pv));

	auto result = dev_->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(screen_vertex_buffer_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	PeraVertex* map = nullptr;
	screen_vertex_buffer_->Map(0, nullptr, (void**)&map);
	std::copy(std::begin(pv), std::end(pv), map);
	screen_vertex_buffer_->Unmap(0, nullptr);

	screen_vertex_buffer_view_.BufferLocation = screen_vertex_buffer_->GetGPUVirtualAddress();
	screen_vertex_buffer_view_.SizeInBytes = sizeof(pv);
	screen_vertex_buffer_view_.StrideInBytes = sizeof(PeraVertex);

	return true;
}

bool Dx12Wrapper::CreatePeraPipeline() {
	D3D12_DESCRIPTOR_RANGE desc_range[4] = {};

	desc_range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // b0
	desc_range[0].BaseShaderRegister = 0;
	desc_range[0].NumDescriptors = 1;

	desc_range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // t0
	desc_range[1].BaseShaderRegister = 0;
	desc_range[1].NumDescriptors = 1;

	desc_range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // t1
	desc_range[2].BaseShaderRegister = 1;
	desc_range[2].NumDescriptors = 1;
	
	// �[�x�l�e�N�X�`��
	desc_range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // t2
	desc_range[3].BaseShaderRegister = 2;
	desc_range[3].NumDescriptors = 1;

	D3D12_ROOT_PARAMETER root_param[4] = {};

	root_param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	root_param[0].DescriptorTable.pDescriptorRanges = &desc_range[0];
	root_param[0].DescriptorTable.NumDescriptorRanges = 1;

	root_param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	root_param[1].DescriptorTable.pDescriptorRanges = &desc_range[1];
	root_param[1].DescriptorTable.NumDescriptorRanges = 1;

	root_param[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_param[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	root_param[2].DescriptorTable.pDescriptorRanges = &desc_range[2];
	root_param[2].DescriptorTable.NumDescriptorRanges = 1;

	root_param[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_param[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	root_param[3].DescriptorTable.pDescriptorRanges = &desc_range[3];
	root_param[3].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC sampler_desc = CD3DX12_STATIC_SAMPLER_DESC(0);
	sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	D3D12_ROOT_SIGNATURE_DESC root_sig_desc = {};
	root_sig_desc.NumParameters = 4;
	root_sig_desc.pParameters = root_param;
	root_sig_desc.NumStaticSamplers = 1;
	root_sig_desc.pStaticSamplers = &sampler_desc;
	root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> root_sig_blob = nullptr;
	ComPtr<ID3DBlob> error_blob = nullptr;

	auto result = D3D12SerializeRootSignature(
		&root_sig_desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		root_sig_blob.ReleaseAndGetAddressOf(),
		error_blob.ReleaseAndGetAddressOf()
	);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	result = dev_->CreateRootSignature(
		0,
		root_sig_blob->GetBufferPointer(),
		root_sig_blob->GetBufferSize(),
		IID_PPV_ARGS(screen_root_signature_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	ComPtr<ID3DBlob> vs_blob = nullptr;
	ComPtr<ID3DBlob> ps_blob = nullptr;

	result = D3DCompileFromFile(
		L"PeraVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PeraVS",
		"vs_5_0",
		0,
		0,
		vs_blob.ReleaseAndGetAddressOf(),
		error_blob.ReleaseAndGetAddressOf()
	);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	result = D3DCompileFromFile(
		L"PeraPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PeraPS",
		"ps_5_0",
		0,
		0,
		ps_blob.ReleaseAndGetAddressOf(),
		error_blob.ReleaseAndGetAddressOf()
	);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	D3D12_INPUT_ELEMENT_DESC input_elem_desc[2] = {
		{
			"POSITION", 
			0, 
			DXGI_FORMAT_R32G32B32_FLOAT, 
			0, 
			D3D12_APPEND_ALIGNED_ELEMENT, 
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 
			0 
		},
		{ 
			"TEXCOORD", 
			0, 
			DXGI_FORMAT_R32G32_FLOAT, 
			0, 
			D3D12_APPEND_ALIGNED_ELEMENT, 
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 
			0 
		}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gps_desc = {};
	gps_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	gps_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
	gps_desc.DepthStencilState.DepthEnable = false;
	gps_desc.DepthStencilState.StencilEnable = false;
	gps_desc.InputLayout.NumElements = _countof(input_elem_desc);
	gps_desc.InputLayout.pInputElementDescs = input_elem_desc;
	gps_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gps_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gps_desc.NumRenderTargets = 1;
	gps_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gps_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gps_desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gps_desc.SampleDesc.Count = 1;
	gps_desc.SampleDesc.Quality = 0;
	gps_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	gps_desc.pRootSignature = screen_root_signature_.Get();

	result = dev_->CreateGraphicsPipelineState(
		&gps_desc,
		IID_PPV_ARGS(screen_pipeline_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	result = D3DCompileFromFile(
		L"PeraPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VerticalBokehPS",
		"ps_5_0",
		0,
		0,
		ps_blob.ReleaseAndGetAddressOf(),
		error_blob.ReleaseAndGetAddressOf()
	);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	gps_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());

	result = dev_->CreateGraphicsPipelineState(
		&gps_desc,
		IID_PPV_ARGS(screen_pipeline_2_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	return true;
}

bool Dx12Wrapper::CreateDepthSRV()
{
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.NodeMask = 0;
	heap_desc.NumDescriptors = 1;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = dev_->CreateDescriptorHeap(
		&heap_desc,
		IID_PPV_ARGS(depth_srv_heap_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	auto handle = depth_srv_heap_->GetCPUDescriptorHandleForHeapStart();

	dev_->CreateShaderResourceView(
		depth_buffer_.Get(),
		&srv_desc,
		handle
	);
}
