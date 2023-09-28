#include <cassert>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include "Dx12Wrapper.h"
#include "Game.h"
#include "Helper.h"
#include "ModelLoader.h"
#include "Renderer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"

using namespace Microsoft::WRL;
// "-" オペランドが使えないため
using namespace DirectX;

Dx12Wrapper::Dx12Wrapper(HWND hwnd) :
	hwnd_(hwnd),
	eye_(0, 15, -50),
	target_(0, 12, 0),
	up_(0, 1, 0),
	view_matrix_(DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&eye_), DirectX::XMLoadFloat3(&target_), DirectX::XMLoadFloat3(&up_)))
{
}

Dx12Wrapper::~Dx12Wrapper()
{
}

bool Dx12Wrapper::Initialize()
{
	Game game;
	window_size_ = game.GetWindowSize();

	light_ = std::make_unique<Light>();

	if (FAILED(InitializeDebug()))
	{
		assert(0);
		return false;
	}

	if (FAILED(InitializeDXGIDevice()))
	{
		assert(0);
		return false;
	}

	if (FAILED(InitializeCommand()))
	{
		assert(0);
		return false;
	}

	if (FAILED(CreateSwapChain()))
	{
		assert(0);
		return false;
	}

	if (FAILED(InitializeRenderer()))
	{
		assert(0);
		return false;
	}

	if (!CreateDepthBuffer())
	{
		return false;
	}

	if (!CreateDSV())
	{
		return false;
	}

	if (!CreateDepthSRV())
	{
		return false;
	}

	if (FAILED(CreateFence()))
	{
		assert(0);
		return false;
	}

	if (FAILED(CreateSceneView()))
	{
		assert(0);
		return false;
	}

	if (FAILED(CreateLight()))
	{
		assert(0);
		return false;
	}

	if (!CreateEffectResourceAndView())
	{
		return false;
	}

	if (!CreatePeraConstBufferAndView())
	{
		return false;
	}

	if (!CreatePeraVerTex())
	{
		return false;
	}

	if (!InitializeImGui())
	{
		return false;
	}

	return true;
}

void Dx12Wrapper::SetSceneCB()
{
	ID3D12DescriptorHeap* heaps[] = { scene_cbv_heap_.Get() };

	cmd_list_->SetDescriptorHeaps(1, heaps);
	cmd_list_->SetGraphicsRootDescriptorTable(0, scene_cbv_heap_->GetGPUDescriptorHandleForHeapStart());
}

void Dx12Wrapper::SetCameraSetting()
{
	Game game;
	auto w_size = game.GetWindowSize();

	scene_matrix_->eye = eye_;
	scene_matrix_->view = view_matrix_;
	scene_matrix_->proj = DirectX::XMMatrixPerspectiveFovLH(
		fov_,
		static_cast<float>(w_size.cx) / static_cast<float>(w_size.cy),
		0.1f,
		1000.0f
	);

	auto light_vec = DirectX::XMLoadFloat3(&light_->GetState()->direction);

	auto light_pos = DirectX::XMLoadFloat3(&target_)
		- DirectX::XMVector3Normalize(light_vec)
		* DirectX::XMVector3Length(DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&target_), DirectX::XMLoadFloat3(&eye_))).m128_f32[0];

	DirectX::XMFLOAT4 plane_normal_vec = { 0.0f, 1.0f, 0.0f, 0.0f };

	scene_matrix_->shadow = DirectX::XMMatrixShadow(
		DirectX::XMLoadFloat4(&plane_normal_vec),
		-light_vec
	);

	scene_matrix_->light_view =
		DirectX::XMMatrixLookAtLH(light_pos, DirectX::XMLoadFloat3(&target_), DirectX::XMLoadFloat3(&up_))
		* DirectX::XMMatrixOrthographicLH(320.0f, 180.0f, 0.1f, 1000.0f);
}

void Dx12Wrapper::SetCommonBuffer(UINT scene_index, UINT light_index, UINT depth_index)
{
	ID3D12DescriptorHeap* scene_heaps[] = { scene_cbv_heap_.Get() };

	cmd_list_->SetDescriptorHeaps(1, scene_heaps);
	cmd_list_->SetGraphicsRootDescriptorTable(scene_index, scene_cbv_heap_->GetGPUDescriptorHandleForHeapStart());

	ID3D12DescriptorHeap* light_heaps[] = { light_csv_heap_.Get() };

	cmd_list_->SetDescriptorHeaps(1, light_heaps);
	cmd_list_->SetGraphicsRootDescriptorTable(light_index, light_csv_heap_->GetGPUDescriptorHandleForHeapStart());

	// 一旦ここでシャドウマップテクスチャをセット
	cmd_list_->SetDescriptorHeaps(1, depth_srv_heap_.GetAddressOf());
	auto handle = depth_srv_heap_->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	cmd_list_->SetGraphicsRootDescriptorTable(depth_index, handle);
}

void Dx12Wrapper::ExecuteCommand()
{
	cmd_list_->Close();

	ID3D12CommandList* cmdLists[] = { cmd_list_.Get() };
	cmd_queue_->ExecuteCommandLists(1, cmdLists);

	cmd_queue_->Signal(fence_.Get(), ++fence_val_);

	if (fence_->GetCompletedValue() < fence_val_)
	{
		auto event = CreateEvent(nullptr, false, false, nullptr);
		fence_->SetEventOnCompletion(fence_val_, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}

	cmd_allocator_->Reset();
	cmd_list_->Reset(cmd_allocator_.Get(), nullptr);
}

void Dx12Wrapper::CreateImguiWindow()
{
	// imgui 描画前処理
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// imgui ウィンドウ描画
	ImGui::Begin("DirectX12 Test Menu");
	ImGui::SetWindowSize(ImVec2(400, 500), ImGuiCond_::ImGuiCond_FirstUseEver);

	ImGui::End();
}

void Dx12Wrapper::RenderImgui()
{
	ImGui::Render();

	cmd_list_->SetDescriptorHeaps(1, imgui_heap_.GetAddressOf());

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list_.Get());
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

	// ライトデプス
	res_desc.Width = 1024;
	res_desc.Height = 1024;

	result = dev_->CreateCommittedResource
	(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clear_val,
		IID_PPV_ARGS(light_depth_buffer_.ReleaseAndGetAddressOf())
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
	// 0番は通常深度、1番はライト深度
	auto result = CreateDescriptorHeapWrapper(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, dsv_heap_);

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// 深度ステンシルビューの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
	dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Flags = D3D12_DSV_FLAG_NONE;

	auto handle = dsv_heap_->GetCPUDescriptorHandleForHeapStart();

	// 通常深度
	dev_->CreateDepthStencilView(depth_buffer_.Get(), &dsv_desc, handle);

	handle.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// ライト深度
	dev_->CreateDepthStencilView(light_depth_buffer_.Get(), &dsv_desc, handle);

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
		cmd_queue_.Get(),
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

	//フィーチャレベル列挙
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};


	// ハードウェアアダプタの検索
	ComPtr<IDXGIAdapter1> useAdapter;
	D3D_FEATURE_LEVEL level;
	{
		UINT adapterIndex = 0;
		ComPtr<IDXGIAdapter1> adapter;
		while (DXGI_ERROR_NOT_FOUND != dxgi_factory_->EnumAdapters1(adapterIndex, &adapter))
		{
			DXGI_ADAPTER_DESC1 desc1{};
			adapter->GetDesc1(&desc1);
			++adapterIndex;
			if (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				continue;
			}

			for (auto l : levels)
			{
				// D3D12は使用可能か
				result = D3D12CreateDevice(adapter.Get(), l, __uuidof(ID3D12Device), nullptr);
				if (SUCCEEDED(result))
				{
					level = l;
					break;
				}
			}
			if (SUCCEEDED(result))
			{
				break;
			}
		}
		adapter.As(&useAdapter); // 使用するアダプター
	}

	// Direct3Dデバイスの作成
	result = D3D12CreateDevice(useAdapter.Get(), level, IID_PPV_ARGS(dev_.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		return result;
	}

	return result;
}

HRESULT Dx12Wrapper::InitializeCommand()
{
	auto result = dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmd_allocator_.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	result = dev_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_allocator_.Get(), nullptr, IID_PPV_ARGS(cmd_list_.ReleaseAndGetAddressOf()));
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

	result = dev_->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(cmd_queue_.ReleaseAndGetAddressOf()));

	assert(SUCCEEDED(result));

	return result;
}

HRESULT Dx12Wrapper::CreateSceneView()
{
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	auto result = swap_chain_->GetDesc1(&swapchainDesc);

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff);

	// 定数バッファの作成
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

	result = CreateDescriptorHeapWrapper(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, scene_cbv_heap_);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	auto handle = scene_cbv_heap_->GetCPUDescriptorHandleForHeapStart();

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

	//SetCameraSetting();

	return result;
}

HRESULT Dx12Wrapper::CreateLight()
{
	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(AligmentedValue(sizeof(LightState), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

	// 定数バッファの作成
	auto result = dev_->CreateCommittedResource
	(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(light_const_buffer_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	result = CreateDescriptorHeapWrapper(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, light_csv_heap_);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	auto handle = light_csv_heap_->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC view_desc = {};
	view_desc.BufferLocation = light_const_buffer_->GetGPUVirtualAddress();
	view_desc.SizeInBytes = light_const_buffer_->GetDesc().Width;

	dev_->CreateConstantBufferView(&view_desc, handle);

	LightState* mapped_light = nullptr;
	result = light_const_buffer_->Map(0, nullptr, (void**)&mapped_light);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	*mapped_light = *(light_->GetState());

	light_const_buffer_->Unmap(0, nullptr);

	return result;
}

HRESULT Dx12Wrapper::InitializeDebug()
{
	ID3D12Debug* debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
	{
		debug->EnableDebugLayer();
		debug->Release();
	}
	return S_OK;
}

HRESULT Dx12Wrapper::InitializeRenderer()
{
	// 修正予定
	renderer_.reset(new Renderer(this));
	renderer_->Initialize();
	model_loader_.reset(new ModelLoader(*renderer_));
	resource_manager_.reset(new ResourceManager(dev_, std::move(model_loader_)));
	return S_OK;
}

HRESULT Dx12Wrapper::CreateFence()
{
	if (FAILED(dev_->CreateFence(fence_val_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf()))))
	{
		assert(0);
		return E_FAIL;
	}
	return S_OK;
}

HRESULT Dx12Wrapper::CreateDescriptorHeapWrapper(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT num_descriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ComPtr<ID3D12DescriptorHeap>& heap, UINT node_mask)
{
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.Type = type;
	heap_desc.NodeMask = node_mask;
	heap_desc.NumDescriptors = num_descriptors;
	heap_desc.Flags = flags;

	return dev_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf()));
}

HRESULT Dx12Wrapper::CreateRenderTargetViewWrapper(ID3D12Resource* resource, DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.Format = format;
	rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	dev_->CreateRenderTargetView(resource, &rtv_desc, handle);
	return S_OK;
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

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	result = CreateDescriptorHeapWrapper(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pera_cbv_heap_);

	if (FAILED(result))
	{
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

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	std::copy(weights.begin(), weights.end(), mapped);
	pera_const_buffer_->Unmap(0, nullptr);
}

bool Dx12Wrapper::CreateEffectResourceAndView()
{
	//effect_resource_ = CreateTextureFromFile("../normal/crack_n.png");
	effect_resource_ = resource_manager_->GetTextureFromPath("../normal/normalmap.jpg");

	auto result = CreateDescriptorHeapWrapper(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, effect_srv_heap_);

	if (FAILED(result))
	{
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

bool Dx12Wrapper::CreatePeraVerTex()
{
	struct PeraVertex
	{
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

	if (FAILED(result))
	{
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

bool Dx12Wrapper::CreateDepthSRV()
{
	// 0番は通常の深度、1番はライト深度
	auto result = CreateDescriptorHeapWrapper(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, depth_srv_heap_);

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	auto handle = depth_srv_heap_->GetCPUDescriptorHandleForHeapStart();

	// 通常深度テクスチャ用
	dev_->CreateShaderResourceView(
		depth_buffer_.Get(),
		&srv_desc,
		handle
	);

	handle.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ライト深度テクスチャ用
	dev_->CreateShaderResourceView(
		light_depth_buffer_.Get(),
		&srv_desc,
		handle
	);

	return true;
}

bool Dx12Wrapper::InitializeImGui()
{
	// ディスクリプタヒープの作成
	auto result = CreateDescriptorHeapWrapper(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, imgui_heap_);

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// 初期化
	if (ImGui::CreateContext() == nullptr)
	{
		assert(0);
		return false;
	}

	// Windows用の初期化
	bool bln_result = ImGui_ImplWin32_Init(hwnd_);
	if (!bln_result)
	{
		assert(0);
		return false;
	}

	// DirectX12用の初期化
	bln_result = ImGui_ImplDX12_Init
	(
		dev_.Get(),
		3,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		imgui_heap_.Get(),
		imgui_heap_->GetCPUDescriptorHandleForHeapStart(),
		imgui_heap_->GetGPUDescriptorHandleForHeapStart()
	);
}
