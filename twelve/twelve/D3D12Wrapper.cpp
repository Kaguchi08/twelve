#include "D3D12Wrapper.h"
#include <cassert>
#include "FileUtil.h" 
#include "ResourceUploadBatch.h"
#include "DDSTextureLoader.h"
#include "VertexTypes.h"

D3D12Wrapper::D3D12Wrapper()
	: m_hWnd(nullptr)
	, m_pFactory(nullptr)
	, m_pDevice(nullptr)
	, m_pQueue(nullptr)
	, m_pSwapChain(nullptr)
	, m_pCmdList(nullptr)
	, m_pHeapRTV(nullptr)
	, m_pFence(nullptr)
	, m_FenceEvent(nullptr)
	, m_FrameIndex(0)
{
	for (auto i = 0u; i < Constants::FrameCount; ++i)
	{
		m_pColorBuffer[i] = nullptr;
		m_pCmdAllocator[i] = nullptr;
		m_FenceCounter[i] = 0;
	}
}

D3D12Wrapper::~D3D12Wrapper()
{
}

bool D3D12Wrapper::Initialize(HWND hWind)
{
	m_hWnd = hWind;

	InitializeDebug();

	// DXGIファクトリーの生成
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_pFactory.GetAddressOf()));

	if (FAILED(result))
	{
		return false;
	}

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

	// デバイスの生成
	result = D3D12CreateDevice(useAdapter.Get(), level, IID_PPV_ARGS(m_pDevice.GetAddressOf()));
	if (FAILED(result))
	{
		return false;
	}

	// コマンドキューの生成
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		result = m_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(m_pQueue.GetAddressOf()));

		if (FAILED(result))
		{
			return false;
		}
	}

	// スワップチェインの生成
	{
		// スワップチェインの設定.
		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferDesc.Width = Constants::WindowWidth;
		desc.BufferDesc.Height = Constants::WindowHeight;
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
		auto result = m_pFactory->CreateSwapChain(m_pQueue.Get(), &desc, pSwapChain.GetAddressOf());

		if (FAILED(result))
		{
			return false;
		}

		// IDXGISwapChain3 を取得
		result = pSwapChain.As(&m_pSwapChain);

		if (FAILED(result))
		{
			return false;
		}

		// バックバッファ番号を取得
		m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

		// 不要になったので解放
		pSwapChain.Reset();
	}

	// コマンドアロケータの生成
	{
		for (auto i = 0u; i < Constants::FrameCount; ++i)
		{
			result = m_pDevice->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(m_pCmdAllocator[i].GetAddressOf()));
			if (FAILED(result))
			{
				return false;
			}
		}
	}

	// コマンドリストの生成
	{
		result = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCmdAllocator[m_FrameIndex].Get(), nullptr, IID_PPV_ARGS(m_pCmdList.GetAddressOf()));

		if (FAILED(result))
		{
			return false;
		}
	}

	// レンダーターゲットビューの生成
	{
		// ディスクリプタヒープの設定
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = Constants::FrameCount;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;

		// ディスクリプタヒープを生成
		result = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pHeapRTV.GetAddressOf()));
		if (FAILED(result))
		{
			return false;
		}

		auto handle = m_pHeapRTV->GetCPUDescriptorHandleForHeapStart();
		auto incrementSize = m_pDevice
			->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (auto i = 0u; i < Constants::FrameCount; ++i)
		{
			result = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pColorBuffer[i].GetAddressOf()));
			if (FAILED(result))
			{
				return false;
			}

			D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
			viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = 0;
			viewDesc.Texture2D.PlaneSlice = 0;

			// レンダーターゲットビューの生成
			m_pDevice->CreateRenderTargetView(m_pColorBuffer[i].Get(), &viewDesc, handle);

			m_HandleRTV[i] = handle;
			handle.ptr += incrementSize;
		}
	}

	// 深度ステンシルバッファの生成
	{
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_DEFAULT;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0;
		resDesc.Width = Constants::WindowWidth;
		resDesc.Height = Constants::WindowHeight;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_D32_FLOAT;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0;
		clearValue.DepthStencil.Stencil = 0;

		result = m_pDevice->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(m_pDepthBuffer.GetAddressOf()));
		if (FAILED(result))
		{
			return false;
		}

		// ディスクリプタヒープの設定
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;

		result = m_pDevice->CreateDescriptorHeap(
			&heapDesc,
			IID_PPV_ARGS(m_pHeapDSV.GetAddressOf()));
		if (FAILED(result))
		{
			return false;
		}

		auto handle = m_pHeapDSV->GetCPUDescriptorHandleForHeapStart();
		auto incrementSize = m_pDevice
			->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
		viewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipSlice = 0;
		viewDesc.Flags = D3D12_DSV_FLAG_NONE;

		m_pDevice->CreateDepthStencilView(m_pDepthBuffer.Get(), &viewDesc, handle);

		m_HandleDSV = handle;
	}

	// フェンスの生成
	{
		// フェンスカウンターをリセット
		for (auto i = 0u; i < Constants::FrameCount; ++i)
		{
			m_FenceCounter[i] = 0;
		}

		// フェンスの生成
		result = m_pDevice->CreateFence(
			m_FenceCounter[m_FrameIndex],
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(m_pFence.GetAddressOf()));
		if (FAILED(result))
		{
			return false;
		}

		m_FenceCounter[m_FrameIndex]++;

		// イベントの生成
		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_FenceEvent == nullptr)
		{
			return false;
		}
	}

	// コマンドリストを閉じる
	m_pCmdList->Close();

	return true;
}

void D3D12Wrapper::Terminate()
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
	m_pHeapRTV.Reset();
	for (auto i = 0u; i < Constants::FrameCount; ++i)
	{
		m_pColorBuffer[i].Reset();
	}

	// コマンドリストの破棄
	m_pCmdList.Reset();

	// コマンドアロケータの破棄
	for (auto i = 0u; i < Constants::FrameCount; ++i)
	{
		m_pCmdAllocator[i].Reset();
	}

	// スワップチェインの破棄
	m_pSwapChain.Reset();

	// コマンドキューの破棄
	m_pQueue.Reset();

	// デバイスの破棄
	m_pDevice.Reset();
}

void D3D12Wrapper::Render()
{
	// 更新処理
	{
		m_RotateAngle += 0.005f;
		m_CBV[m_FrameIndex].pBuffer->World = DirectX::XMMatrixRotationY(m_RotateAngle);
	}

	// コマンドの記録を開始
	m_pCmdAllocator[m_FrameIndex]->Reset();
	m_pCmdList->Reset(m_pCmdAllocator[m_FrameIndex].Get(), nullptr);

	// リソースバリアの設定
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_pColorBuffer[m_FrameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// リソースバリア
	m_pCmdList->ResourceBarrier(1, &barrier);

	// レンダーゲットの設定
	m_pCmdList->OMSetRenderTargets(1, &m_HandleRTV[m_FrameIndex], FALSE, &m_HandleDSV);

	// クリアカラーの設定
	float clearColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };

	// レンダーターゲットビューをクリア
	m_pCmdList->ClearRenderTargetView(m_HandleRTV[m_FrameIndex], clearColor, 0, nullptr);

	// 深度ステンシルビューをクリア
	m_pCmdList->ClearDepthStencilView(m_HandleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 描画処理
	{
		m_pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
		m_pCmdList->SetDescriptorHeaps(1, m_pHeapCBV_SRV_UAV.GetAddressOf());
		m_pCmdList->SetGraphicsRootConstantBufferView(0, m_CBV[m_FrameIndex].Desc.BufferLocation);
		m_pCmdList->SetGraphicsRootDescriptorTable(1, m_Texture.HandleGPU);
		m_pCmdList->SetPipelineState(m_pPSO.Get());

		m_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pCmdList->IASetVertexBuffers(0, 1, &m_VBV);
		m_pCmdList->IASetIndexBuffer(&m_IBV);
		m_pCmdList->RSSetViewports(1, &m_Viewport);
		m_pCmdList->RSSetScissorRects(1, &m_Scissor);

		auto count = static_cast<uint32_t>(m_Meshes[0].Indices.size());
		m_pCmdList->DrawIndexedInstanced(count, 1, 0, 0, 0);
	}

	// リソースバリアの設定
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_pColorBuffer[m_FrameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// リソースバリア
	m_pCmdList->ResourceBarrier(1, &barrier);

	// コマンドの記録を終了
	m_pCmdList->Close();

	// コマンドを実行
	ID3D12CommandList* ppCmdLists[] = { m_pCmdList.Get() };
	m_pQueue->ExecuteCommandLists(1, ppCmdLists);

	// 画面に表示
	Present(1);
}

bool D3D12Wrapper::InitializeGraphicsPipeline()
{
	// メッシュをロード
	{
		std::wstring path;
		if (!SearchFilePath(L"Assets/teapot/teapot.obj", path))
		{
			return false;
		}

		if (!LoadMesh(path.c_str(), m_Meshes, m_Materials))
		{
			return false;
		}
	}

	// 頂点バッファの生成
	{
		auto size = sizeof(MeshVertex) * m_Meshes[0].Vertices.size();
		auto vertices = m_Meshes[0].Vertices.data();

		// ヒーププロパティ
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		// リソースの設定
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// リソースを生成
		auto hr = m_pDevice->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_pVB.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}

		// マッピングする
		void* ptr = nullptr;
		hr = m_pVB->Map(0, nullptr, &ptr);
		if (FAILED(hr))
		{
			return false;
		}

		// 頂点データをマッピング先に設定
		memcpy(ptr, vertices, size);

		// マッピング解除
		m_pVB->Unmap(0, nullptr);

		// 頂点バッファビューの設定
		m_VBV.BufferLocation = m_pVB->GetGPUVirtualAddress();
		m_VBV.SizeInBytes = static_cast<UINT>(size);
		m_VBV.StrideInBytes = static_cast<UINT>(sizeof(MeshVertex));
	}

	// インデックスバッファの生成
	{
		auto size = sizeof(uint32_t) * m_Meshes[0].Indices.size();
		auto indices = m_Meshes[0].Indices.data();

		// ヒーププロパティ
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		// リソースの設定
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// リソースを生成
		auto hr = m_pDevice->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_pIB.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}

		// マッピングする
		void* ptr = nullptr;
		hr = m_pIB->Map(0, nullptr, &ptr);
		if (FAILED(hr))
		{
			return false;
		}

		// インデックスデータをマッピング先に設定
		memcpy(ptr, indices, size);

		// マッピング解除
		m_pIB->Unmap(0, nullptr);

		// インデックスバッファビューの設定
		m_IBV.BufferLocation = m_pIB->GetGPUVirtualAddress();
		m_IBV.Format = DXGI_FORMAT_R32_UINT;
		m_IBV.SizeInBytes = static_cast<UINT>(size);
	}

	// CBV／SRV／UAV用ディスクリプタヒープの生成
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 3 * Constants::FrameCount;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NodeMask = 0;

		auto hr = m_pDevice->CreateDescriptorHeap(
			&desc,
			IID_PPV_ARGS(m_pHeapCBV_SRV_UAV.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}
	}

	// 定数バッファの生成
	{
		// ヒーププロパティ
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		// リソースの設定
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = sizeof(Transform);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		auto incrementSize = m_pDevice
			->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		for (auto i = 0; i < Constants::FrameCount * 2; ++i)
		{
			// リソース生成.
			auto hr = m_pDevice->CreateCommittedResource(
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(m_pCB[i].GetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}

			auto address = m_pCB[i]->GetGPUVirtualAddress();
			auto handleCPU = m_pHeapCBV_SRV_UAV->GetCPUDescriptorHandleForHeapStart();
			auto handleGPU = m_pHeapCBV_SRV_UAV->GetGPUDescriptorHandleForHeapStart();

			handleCPU.ptr += incrementSize * i;
			handleGPU.ptr += incrementSize * i;

			// 定数バッファビューの設定
			m_CBV[i].HandleCPU = handleCPU;
			m_CBV[i].HandleGPU = handleGPU;
			m_CBV[i].Desc.BufferLocation = address;
			m_CBV[i].Desc.SizeInBytes = sizeof(Transform);

			// 定数バッファビューを生成
			m_pDevice->CreateConstantBufferView(&m_CBV[i].Desc, handleCPU);

			// マッピング
			hr = m_pCB[i]->Map(0, nullptr, reinterpret_cast<void**>(&m_CBV[i].pBuffer));
			if (FAILED(hr))
			{
				return false;
			}

			auto eyePos = DirectX::XMVectorSet(0.0f, 1.0f, 2.0f, 0.0f);
			auto targetPos = DirectX::XMVectorZero();
			auto upward = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

			auto fovY = DirectX::XMConvertToRadians(37.5f);
			auto aspect = static_cast<float>(Constants::WindowWidth) / static_cast<float>(Constants::WindowHeight);

			// 変換行列の設定
			m_CBV[i].pBuffer->World = DirectX::XMMatrixIdentity();
			m_CBV[i].pBuffer->View = DirectX::XMMatrixLookAtRH(eyePos, targetPos, upward);
			m_CBV[i].pBuffer->Proj = DirectX::XMMatrixPerspectiveFovRH(fovY, aspect, 1.0f, 1000.0f);
		}
	}

	// ルートシグニチャの生成
	{
		auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
		flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
		flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		// ルートパラメータの設定
		D3D12_ROOT_PARAMETER param[2] = {};
		param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		param[0].Descriptor.ShaderRegister = 0;
		param[0].Descriptor.RegisterSpace = 0;
		param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		D3D12_DESCRIPTOR_RANGE range = {};
		range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range.NumDescriptors = 1;
		range.BaseShaderRegister = 0;
		range.RegisterSpace = 0;
		range.OffsetInDescriptorsFromTableStart = 0;

		param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[1].DescriptorTable.NumDescriptorRanges = 1;
		param[1].DescriptorTable.pDescriptorRanges = &range;
		param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// スタティックサンプラーの設定
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.MipLODBias = D3D12_DEFAULT_MIP_LOD_BIAS;
		sampler.MaxAnisotropy = 1;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = -D3D12_FLOAT32_MAX;
		sampler.MaxLOD = +D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// ルートシグニチャの設定
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = 2;
		desc.NumStaticSamplers = 1;
		desc.pParameters = param;
		desc.pStaticSamplers = &sampler;
		desc.Flags = flag;

		ComPtr<ID3DBlob> pBlob;
		ComPtr<ID3DBlob> pErrorBlob;

		// シリアライズ
		auto hr = D3D12SerializeRootSignature(
			&desc,
			D3D_ROOT_SIGNATURE_VERSION_1_0,
			pBlob.GetAddressOf(),
			pErrorBlob.GetAddressOf());
		if (FAILED(hr))
		{
			return false;
		}

		// ルートシグニチャを生成
		hr = m_pDevice->CreateRootSignature(
			0,
			pBlob->GetBufferPointer(),
			pBlob->GetBufferSize(),
			IID_PPV_ARGS(m_pRootSignature.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}
	}

	// パイプラインステートの生成
	{
		// ラスタライザーステートの設定
		D3D12_RASTERIZER_DESC descRS;
		descRS.FillMode = D3D12_FILL_MODE_SOLID;
		descRS.CullMode = D3D12_CULL_MODE_NONE;
		descRS.FrontCounterClockwise = FALSE;
		descRS.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		descRS.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		descRS.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		descRS.DepthClipEnable = FALSE;
		descRS.MultisampleEnable = FALSE;
		descRS.AntialiasedLineEnable = FALSE;
		descRS.ForcedSampleCount = 0;
		descRS.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		// レンダーターゲットのブレンド設定
		D3D12_RENDER_TARGET_BLEND_DESC descRTBS = {
			FALSE, FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL
		};

		// ブレンドステートの設定
		D3D12_BLEND_DESC descBS;
		descBS.AlphaToCoverageEnable = FALSE;
		descBS.IndependentBlendEnable = FALSE;
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			descBS.RenderTarget[i] = descRTBS;
		}

		// 深度ステンシルステートの設定.
		D3D12_DEPTH_STENCIL_DESC descDSS = {};
		descDSS.DepthEnable = TRUE;
		descDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		descDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		descDSS.StencilEnable = FALSE;

		ComPtr<ID3DBlob> pVSBlob;
		ComPtr<ID3DBlob> pPSBlob;

		std::wstring vsPath;
		std::wstring psPath;

		if (!SearchFilePath(L"SimpleTexVS.cso", vsPath))
		{
			return false;
		}

		if (!SearchFilePath(L"SimpleTexPS.cso", psPath))
		{
			return false;
		}

		// 頂点シェーダ読み込み.
		auto hr = D3DReadFileToBlob(vsPath.c_str(), pVSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			return false;
		}

		// ピクセルシェーダ読み込み.
		hr = D3DReadFileToBlob(psPath.c_str(), pPSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			return false;
		}

		// パイプラインステートの設定
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.InputLayout = MeshVertex::InputLayout;
		desc.pRootSignature = m_pRootSignature.Get();
		desc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
		desc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
		desc.RasterizerState = descRS;
		desc.BlendState = descBS;
		desc.DepthStencilState = descDSS;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		// パイプラインステートを生成
		hr = m_pDevice->CreateGraphicsPipelineState(
			&desc,
			IID_PPV_ARGS(m_pPSO.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}
	}

	// テクスチャの生成
	{
		// ファイルパスを検索する
		std::wstring texturePath;
		if (!SearchFilePath(L"Assets/teapot/default.dds", texturePath))
		{
			return false;
		}

		DirectX::ResourceUploadBatch batch(m_pDevice.Get());
		batch.Begin();

		// リソースを生成
		auto hr = DirectX::CreateDDSTextureFromFile(
			m_pDevice.Get(),
			batch,
			texturePath.c_str(),
			m_Texture.pResource.GetAddressOf(),
			true);
		if (FAILED(hr))
		{
			return false;
		}

		// コマンドを実行
		auto future = batch.End(m_pQueue.Get());

		// コマンドの完了を待機する
		future.wait();

		// インクリメントサイズを取得
		auto incrementSize = m_pDevice
			->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// CPUディスクリプタハンドルとGPUディスクリプタハンドルをディスクリプタヒープから取得
		auto handleCPU = m_pHeapCBV_SRV_UAV->GetCPUDescriptorHandleForHeapStart();
		auto handleGPU = m_pHeapCBV_SRV_UAV->GetGPUDescriptorHandleForHeapStart();

		// テクスチャにディスクリプタを割り当てる
		handleCPU.ptr += incrementSize * Constants::FrameCount * 2;
		handleGPU.ptr += incrementSize * Constants::FrameCount * 2;

		m_Texture.HandleCPU = handleCPU;
		m_Texture.HandleGPU = handleGPU;

		// テクスチャの構成設定を取得
		auto textureDesc = m_Texture.pResource->GetDesc();

		// シェーダリソースビューの設定.
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Format = textureDesc.Format;
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		viewDesc.Texture2D.MostDetailedMip = 0;
		viewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
		viewDesc.Texture2D.PlaneSlice = 0;
		viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		// シェーダリソースビューを生成.
		m_pDevice->CreateShaderResourceView(
			m_Texture.pResource.Get(), &viewDesc, handleCPU);
	}

	// ビューポートとシザー矩形の設定
	{
		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = static_cast<float>(Constants::WindowWidth);
		m_Viewport.Height = static_cast<float>(Constants::WindowHeight);
		m_Viewport.MinDepth = 0.0f;
		m_Viewport.MaxDepth = 1.0f;

		m_Scissor.left = 0;
		m_Scissor.right = Constants::WindowWidth;
		m_Scissor.top = 0;
		m_Scissor.bottom = Constants::WindowHeight;
	}

	return true;
}

void D3D12Wrapper::ReleaseGraphicsResources()
{
	//WaitGPU();

	for (auto i = 0; i < Constants::FrameCount * 2; ++i)
	{
		if (m_pCB[i].Get() != nullptr)
		{
			m_pCB[i]->Unmap(0, nullptr);
			memset(&m_CBV[i], 0, sizeof(m_CBV[i]));
		}
		m_pCB[i].Reset();
	}

	m_pIB.Reset();
	m_pVB.Reset();
	m_pPSO.Reset();
}

void D3D12Wrapper::InitializeDebug()
{
#if defined(DEBUG) || defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debug;
		auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf()));

		// デバッグレイヤーを有効化.
		if (SUCCEEDED(hr))
		{
			debug->EnableDebugLayer();
		}
	}
#endif
}

void D3D12Wrapper::WaitGPU()
{
	assert(m_pQueue != nullptr);
	assert(m_pFence != nullptr);
	assert(m_FenceEvent != nullptr);

	// シグナル処理
	m_pQueue->Signal(m_pFence.Get(), m_FenceCounter[m_FrameIndex]);

	// 完了時にイベントを設定する
	m_pFence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);

	// 待機処理
	WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);

	// カウンターを増やす(正しい？)
	m_FenceCounter[m_FrameIndex]++;

	// デバッグ
	auto x = m_FenceCounter[m_FrameIndex];
}

void D3D12Wrapper::Present(uint32_t interval)
{
	// 画面に表示
	m_pSwapChain->Present(interval, 0);

	// デバッグ
	auto fenceValue = m_pFence->GetCompletedValue();

	// シグナル処理
	const auto currentValue = m_FenceCounter[m_FrameIndex];
	m_pQueue->Signal(m_pFence.Get(), currentValue);

	// バックバッファ番号を更新
	m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// 次のフレームの描画準備がまだであれば待機する
	if (m_pFence->GetCompletedValue() < m_FenceCounter[m_FrameIndex])
	{
		m_pFence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);
		WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
	}

	// デバッグ
	fenceValue = m_pFence->GetCompletedValue();

	// 次のフレームのフェンスカウンターを増やす
	m_FenceCounter[m_FrameIndex] = currentValue + 1;
}
