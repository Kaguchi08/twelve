#include "Dx12Wrapper.h"

#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl.h>

#include <cassert>

#include "Game.h"
#include "Helper.h"
#include "ModelLoader.h"
#include "Renderer.h"
#include "XMFLOAT_Helper.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"

using namespace Microsoft::WRL;
// "-" オペランドが使えないため
using namespace DirectX;

Dx12Wrapper::Dx12Wrapper(HWND hwnd) : m_hWnd(hwnd),
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
	m_WindowSize = game.GetWindowSize();

	light_ = std::make_unique<Light>();

	if (FAILED(InitializeDebug()))
	{
		assert(0);
		return false;
	}

	if (FAILED(InitializeDevice()))
	{
		assert(0);
		return false;
	}

	if (FAILED(InitializeCmdueue()))
	{
		assert(0);
		return false;
	}

	if (FAILED(CreateSwapChain()))
	{
		assert(0);
		return false;
	}

	if (FAILED(InitializeCmdAllocator()))
	{
		assert(0);
		return false;
	}

	if (FAILED(InitializeCmdList()))
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

	if (!CreatePostEffectAndView())
	{
		return false;
	}

	if (!CreateOffScreenVertex())
	{
		return false;
	}

	if (!InitializeImGui())
	{
		return false;
	}

	if (FAILED(CreateFence()))
	{
		assert(0);
		return false;
	}

	m_pCmdList->Close();

	return true;
}

void Dx12Wrapper::Terminate()
{
	// GPU処理の完了を待機
	WaitGPU();

	// イベント破棄
	if (m_FenceEvent != nullptr)
	{
		CloseHandle(m_FenceEvent);
		m_FenceEvent = nullptr;
	}

	// フェンス破棄
	m_pFence.Reset();

	// レンダーターゲットビューの破棄

	// コマンドリストの破棄
	m_pCmdList.Reset();

	// コマンドアロケータの破棄
	m_pCmdAllocator.Reset();

	// スワップチェインの破棄
	m_pSwapChain.Reset();

	// コマンドキューの破棄
	m_pQueue.Reset();

	// デバイスの破棄
	m_pDevice.Reset();
}

void Dx12Wrapper::PrepareRendering()
{
	m_pCmdAllocator->Reset();
	m_pCmdList->Reset(m_pCmdAllocator.Get(), nullptr);
}

void Dx12Wrapper::ExecuteCommand()
{
	m_pCmdList->Close();

	ID3D12CommandList* cmdLists[] = { m_pCmdList.Get() };
	m_pQueue->ExecuteCommandLists(1, cmdLists);
}

void Dx12Wrapper::Present(uint32_t interval)
{
	// 画面に表示
	m_pSwapChain->Present(interval, 0);

	m_pQueue->Signal(m_pFence.Get(), ++m_FenceValue);

	if (m_pFence->GetCompletedValue() < m_FenceValue)
	{
		m_pFence->SetEventOnCompletion(m_FenceValue, m_FenceEvent);
		WaitForSingleObject(m_FenceEvent, INFINITE);
	}
}

void Dx12Wrapper::SetSceneCB()
{
	ID3D12DescriptorHeap* heaps[] = { scene_cbv_heap_.Get() };

	m_pCmdList->SetDescriptorHeaps(1, heaps);
	m_pCmdList->SetGraphicsRootDescriptorTable(0, scene_cbv_heap_->GetGPUDescriptorHandleForHeapStart());
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
		1000.0f);

	auto light_vec = DirectX::XMLoadFloat3(&light_->GetState()->direction);

	auto light_pos = DirectX::XMLoadFloat3(&target_) - DirectX::XMVector3Normalize(light_vec) * DirectX::XMVector3Length(DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&target_), DirectX::XMLoadFloat3(&eye_))).m128_f32[0];

	DirectX::XMFLOAT4 plane_normal_vec = { 0.0f, 1.0f, 0.0f, 0.0f };

	scene_matrix_->shadow = DirectX::XMMatrixShadow(
		DirectX::XMLoadFloat4(&plane_normal_vec),
		-light_vec);

	scene_matrix_->light_view =
		DirectX::XMMatrixLookAtLH(light_pos, DirectX::XMLoadFloat3(&target_), DirectX::XMLoadFloat3(&up_)) * DirectX::XMMatrixOrthographicLH(320.0f, 180.0f, 0.1f, 1000.0f);
}

void Dx12Wrapper::SetCommonBuffer(UINT scene_index, UINT light_index, UINT depth_index)
{
	// シーン定数バッファをセット
	ID3D12DescriptorHeap* scene_heaps[] = { scene_cbv_heap_.Get() };

	m_pCmdList->SetDescriptorHeaps(1, scene_heaps);
	m_pCmdList->SetGraphicsRootDescriptorTable(scene_index, scene_cbv_heap_->GetGPUDescriptorHandleForHeapStart());

	// ライト定数バッファをセット
	ID3D12DescriptorHeap* light_heaps[] = { light_cbv_heap_.Get() };

	m_pCmdList->SetDescriptorHeaps(1, light_heaps);
	m_pCmdList->SetGraphicsRootDescriptorTable(light_index, light_cbv_heap_->GetGPUDescriptorHandleForHeapStart());

	// シャドウマップテクスチャをセット
	m_pCmdList->SetDescriptorHeaps(1, depth_srv_heap_.GetAddressOf());
	auto handle = depth_srv_heap_->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_pCmdList->SetGraphicsRootDescriptorTable(depth_index, handle);
}

void Dx12Wrapper::CreateImguiWindow()
{
	// imgui 描画前処理
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// パラメータウィンドウ
	ImGui::Begin("Menu");
	ImGui::SetWindowSize(ImVec2(500, 400), ImGuiCond_::ImGuiCond_FirstUseEver);

	// FPS
	ImGui::Text("FPS: %f", fps_);

	// ディレクションライト
	static float dir_light[3] = { 1.0f, -1.0f, 1.0f };
	ImGui::SliderFloat3("Direction Light", dir_light, -1.0f, 1.0f);
	static float dir_color[3] = { 1.0f, 1.0f, 1.0f };
	ImGui::ColorEdit3("Direction Light Color", dir_color);

	// アンビエントライト
	static float ambient_light[3] = { 1.0f, 1.0f, 1.0f };
	ImGui::ColorEdit3("Ambient Light", ambient_light);

	// ゲーム終了ボタンは右下に固定
	// ウィンドウのサイズを取得
	ImVec2 windowSize = ImGui::GetWindowSize();
	// 現在のカーソル位置を保存
	ImVec2 oldCursorPos = ImGui::GetCursorPos();
	// ボタンのサイズ
	ImVec2 buttonSize(100, 50);
	// カーソル位置をボタンの位置に設定
	ImGui::SetCursorPos(ImVec2(windowSize.x - buttonSize.x, windowSize.y - buttonSize.y));
	// ゲーム終了
	if (ImGui::Button("Quit Game"))
	{
		PostQuitMessage(0);
	}
	// カーソル位置を復元
	ImGui::SetCursorPos(oldCursorPos);

	// 情報を更新
	SetLightState(DirectX::XMFLOAT3(dir_light), DirectX::XMFLOAT3(dir_color), DirectX::XMFLOAT3(ambient_light));

	ImGui::End();
}

void Dx12Wrapper::RenderImgui()
{
	ImGui::Render();

	m_pCmdList->SetDescriptorHeaps(1, imgui_heap_.GetAddressOf());

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pCmdList.Get());
}

bool Dx12Wrapper::CreateDepthBuffer()
{
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	auto result = m_pSwapChain->GetDesc1(&swap_chain_desc);

	D3D12_RESOURCE_DESC res_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, swap_chain_desc.Width, swap_chain_desc.Height);
	res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_CLEAR_VALUE clear_val(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	result = m_pDevice->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clear_val,
		IID_PPV_ARGS(depth_buffer_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// ライトデプス
	res_desc.Width = kLightDepthMapSize;
	res_desc.Height = kLightDepthMapSize;

	result = m_pDevice->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clear_val,
		IID_PPV_ARGS(light_depth_buffer_.ReleaseAndGetAddressOf()));

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
	m_pDevice->CreateDepthStencilView(depth_buffer_.Get(), &dsv_desc, handle);

	handle.ptr += m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// ライト深度
	m_pDevice->CreateDepthStencilView(light_depth_buffer_.Get(), &dsv_desc, handle);

	return true;
}

HRESULT Dx12Wrapper::CreateSwapChain()
{
	// スワップチェインの設定.
	DXGI_SWAP_CHAIN_DESC desc = {};
	desc.BufferDesc.Width = m_WindowSize.cx;
	desc.BufferDesc.Height = m_WindowSize.cy;
	desc.BufferDesc.RefreshRate.Numerator = 60;
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = Constants::FrameCount;
	desc.OutputWindow = m_hWnd;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// スワップチェインの生成
	ComPtr<IDXGISwapChain> pSwapChain;

	auto result = m_pFactory->CreateSwapChain(
		m_pQueue.Get(),
		&desc,
		pSwapChain.GetAddressOf());

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// IDXGISwapChain3 を取得
	result = pSwapChain.As(&m_pSwapChain);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// 不要になったので解放
	pSwapChain.Reset();

	return result;
}

HRESULT Dx12Wrapper::InitializeDevice()
{
	HRESULT result = S_OK;

	result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_pFactory.ReleaseAndGetAddressOf()));

	// フィーチャレベル列挙
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

		while (DXGI_ERROR_NOT_FOUND != m_pFactory->EnumAdapters1(adapterIndex, &adapter))
		{
			DXGI_ADAPTER_DESC1 desc1{};
			adapter->GetDesc1(&desc1);

			++adapterIndex;

			// ソフトウェアアダプターはスキップ
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

		// 使用するアダプター
		adapter.As(&useAdapter);
	}

	// Direct3Dデバイスの作成
	result = D3D12CreateDevice(useAdapter.Get(), level, IID_PPV_ARGS(m_pDevice.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		return result;
	}

	return result;
}

HRESULT Dx12Wrapper::InitializeCmdueue()
{
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	auto result = m_pDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(m_pQueue.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
	}

	return result;
}

HRESULT Dx12Wrapper::InitializeCmdAllocator()
{
	auto result = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCmdAllocator.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
	}

	return result;
}

HRESULT Dx12Wrapper::InitializeCmdList()
{
	HRESULT result;

	for (auto i = 0u; i < Constants::FrameCount; ++i)
	{
		result = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCmdAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCmdList.ReleaseAndGetAddressOf()));

		if (FAILED(result))
		{
			assert(0);
			return result;
		}
	}

	return result;
}

HRESULT Dx12Wrapper::CreateSceneView()
{
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	auto result = m_pSwapChain->GetDesc1(&swapchainDesc);

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff);

	// 定数バッファの作成
	result = m_pDevice->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(scene_const_buff_.ReleaseAndGetAddressOf()));

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

	m_pDevice->CreateConstantBufferView(&view_desc, handle);

	scene_matrix_ = nullptr;
	result = scene_const_buff_->Map(0, nullptr, (void**)&scene_matrix_);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// SetCameraSetting();

	return result;
}

HRESULT Dx12Wrapper::CreateLight()
{
	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(AligmentedValue(sizeof(LightState), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

	// 定数バッファの作成
	auto result = m_pDevice->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(light_const_buffer_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	result = CreateDescriptorHeapWrapper(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, light_cbv_heap_);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	auto handle = light_cbv_heap_->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC view_desc = {};
	view_desc.BufferLocation = light_const_buffer_->GetGPUVirtualAddress();
	view_desc.SizeInBytes = light_const_buffer_->GetDesc().Width;

	m_pDevice->CreateConstantBufferView(&view_desc, handle);

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

void Dx12Wrapper::SetLightState(DirectX::XMFLOAT3 dir_light, DirectX::XMFLOAT3 dir_color, DirectX::XMFLOAT3 ambient_light)
{
	light_->SetDirection(dir_light);
	light_->SetColor(dir_color);
	light_->SetAmbient(ambient_light);

	LightState* mapped_light = nullptr;
	auto result = light_const_buffer_->Map(0, nullptr, (void**)&mapped_light);

	if (FAILED(result))
	{
		assert(0);
	}

	*mapped_light = *(light_->GetState());

	light_const_buffer_->Unmap(0, nullptr);
}

HRESULT Dx12Wrapper::InitializeDebug()
{
#if defined(DEBUG) || defined(_DEBUG)

	ComPtr<ID3D12Debug> debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf()))))
	{
		debug->EnableDebugLayer();
	}
#endif

	return S_OK;
}

HRESULT Dx12Wrapper::InitializeRenderer()
{
	// 修正予定
	renderer_.reset(new Renderer(this));
	renderer_->Initialize();
	model_loader_.reset(new ModelLoader(*renderer_));
	resource_manager_.reset(new ResourceManager(m_pDevice, std::move(model_loader_)));
	return S_OK;
}

HRESULT Dx12Wrapper::CreateFence()
{
	// フェンスの生成
	auto result = m_pDevice->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// イベントの生成
	m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	if (m_FenceEvent == nullptr)
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

	return m_pDevice->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf()));
}

HRESULT Dx12Wrapper::CreateRenderTargetViewWrapper(ID3D12Resource* resource, DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.Format = format;
	rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	m_pDevice->CreateRenderTargetView(resource, &rtv_desc, handle);
	return S_OK;
}

bool Dx12Wrapper::CreatePostEffectAndView()
{
	auto weights = GetGaussianWeights(8, 5.0f);
	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(AligmentedValue(sizeof(weights[0]) * weights.size(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

	auto result = m_pDevice->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(post_effect_const_buffer_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	result = CreateDescriptorHeapWrapper(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, post_effect_cbv_heap_);

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = post_effect_const_buffer_->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = post_effect_const_buffer_->GetDesc().Width;

	auto handle = post_effect_cbv_heap_->GetCPUDescriptorHandleForHeapStart();

	m_pDevice->CreateConstantBufferView(&cbv_desc, handle);

	float* mapped = nullptr;
	result = post_effect_const_buffer_->Map(0, nullptr, (void**)&mapped);

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	std::copy(weights.begin(), weights.end(), mapped);
	post_effect_const_buffer_->Unmap(0, nullptr);
}

bool Dx12Wrapper::CreateOffScreenVertex()
{
	struct OffScreenVertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};

	OffScreenVertex vertex[4] = {
		{{-1, -1, 0.1}, {0, 1}},
		{{-1, 1, 0.1}, {0, 0}},
		{{1, -1, 0.1}, {1, 1}},
		{{1, 1, 0.1}, {1, 0}} };

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertex));

	auto result = m_pDevice->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(screen_vertex_buffer_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	OffScreenVertex* map = nullptr;
	screen_vertex_buffer_->Map(0, nullptr, (void**)&map);
	std::copy(std::begin(vertex), std::end(vertex), map);
	screen_vertex_buffer_->Unmap(0, nullptr);

	screen_vertex_buffer_view_.BufferLocation = screen_vertex_buffer_->GetGPUVirtualAddress();
	screen_vertex_buffer_view_.SizeInBytes = sizeof(vertex);
	screen_vertex_buffer_view_.StrideInBytes = sizeof(OffScreenVertex);

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
	m_pDevice->CreateShaderResourceView(
		depth_buffer_.Get(),
		&srv_desc,
		handle);

	handle.ptr += m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ライト深度テクスチャ用
	m_pDevice->CreateShaderResourceView(
		light_depth_buffer_.Get(),
		&srv_desc,
		handle);

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
	bool bln_result = ImGui_ImplWin32_Init(m_hWnd);
	if (!bln_result)
	{
		assert(0);
		return false;
	}

	// DirectX12用の初期化
	bln_result = ImGui_ImplDX12_Init(
		m_pDevice.Get(),
		3,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		imgui_heap_.Get(),
		imgui_heap_->GetCPUDescriptorHandleForHeapStart(),
		imgui_heap_->GetGPUDescriptorHandleForHeapStart());
}

void Dx12Wrapper::WaitGPU()
{
	assert(m_pQueue != nullptr);
	assert(m_pFence != nullptr);
	assert(m_FenceEvent != nullptr);

	// シグナル処理
	m_pQueue->Signal(m_pFence.Get(), ++m_FenceValue);

	// 完了時にイベントを設定する..
	m_pFence->SetEventOnCompletion(m_FenceValue, m_FenceEvent);

	// 待機処理
	WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
}
