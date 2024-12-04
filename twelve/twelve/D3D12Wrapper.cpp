#include "D3D12Wrapper.h"

#include <cassert>
#include <ResourceUploadBatch.h>
#include <DDSTextureLoader.h>
#include <CommonStates.h>
#include <VertexTypes.h>
#include <DirectXHelpers.h>
#include <SimpleMath.h>
#include <algorithm>

#include "FileUtil.h" 
#include "Logger.h"

using namespace DirectX::SimpleMath;

namespace
{
	struct Transform
	{
		Matrix World;	// ワールド行列
		Matrix View;	// ビュー行列
		Matrix Proj;	// 射影行列
	};

	struct LightBuffer
	{
		Vector4 LitghtPosition;		// ライトの位置
		Color LightColor;			// ライトから―
		Vector4 CameraPosition;		// カメラの位置
	};

	struct MaterialBuffer
	{
		Vector3 BaseColor;		// 基本色
		float Alpha;			// 透過度
		float Metalic;			// 金属度
		float Shininess;		// 鏡面反射強度
	};

	/// <summary>
	/// 領域の交差判定を計算する
	/// </summary>
	inline int ComputeIntersectionArea
	(
		int ax1, int ay1,
		int ax2, int ay2,
		int bx1, int by1,
		int bx2, int by2
	)
	{
		return std::max(0, std::min(ax2, bx2) - std::max(ax1, bx1))
			* std::max(0, std::min(ay2, by2) - std::max(ay1, by1));
	}
}

D3D12Wrapper::D3D12Wrapper()
	: m_hWnd(nullptr)
	, m_pFactory(nullptr)
	, m_pDevice(nullptr)
	, m_pQueue(nullptr)
	, m_pSwapChain(nullptr)
	, m_FrameIndex(0)
{
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

	// ディスクリプタプールの生成
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};

		desc.NodeMask = 1;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 512;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if (!DescriptorPool::Create(m_pDevice.Get(), &desc, &m_pPool[POOL_TYPE_RES]))
		{
			return false;
		}

		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		desc.NumDescriptors = 256;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if (!DescriptorPool::Create(m_pDevice.Get(), &desc, &m_pPool[POOL_TYPE_SMP]))
		{
			return false;
		}

		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = 512;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		if (!DescriptorPool::Create(m_pDevice.Get(), &desc, &m_pPool[POOL_TYPE_RTV]))
		{
			return false;
		}

		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		desc.NumDescriptors = 512;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		if (!DescriptorPool::Create(m_pDevice.Get(), &desc, &m_pPool[POOL_TYPE_DSV]))
		{
			return false;
		}
	}

	// コマンドリストの生成
	{
		if (!m_CommandList.Init(m_pDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, Constants::FrameCount))
		{
			return false;
		}
	}

	// レンダーターゲットビューの生成
	{
		for (auto i = 0u; i < Constants::FrameCount; ++i)
		{
			if (!m_RenderTarget[i].InitFromBackBuffer(m_pDevice.Get(), m_pPool[POOL_TYPE_RTV], true, i, m_pSwapChain.Get()))
			{
				return false;
			}
		}
	}

	// 深度ステンシルバッファの生成
	{
		if (!m_DepthTarget.Init(m_pDevice.Get(), m_pPool[POOL_TYPE_DSV], Constants::WindowWidth, Constants::WindowHeight, DXGI_FORMAT_D32_FLOAT))
		{
			return false;
		}
	}

	// フェンスの生成
	{
		if (!m_Fence.Init(m_pDevice.Get()))
		{
			return false;
		}
	}

	// ビューポートの設定
	{
		m_Viewport.TopLeftX = 0.0f;
		m_Viewport.TopLeftY = 0.0f;
		m_Viewport.Width = static_cast<float>(Constants::WindowWidth);
		m_Viewport.Height = static_cast<float>(Constants::WindowHeight);
		m_Viewport.MinDepth = 0.0f;
		m_Viewport.MaxDepth = 1.0f;
	}

	// シザー矩形の設定
	{
		m_Scissor.left = 0;
		m_Scissor.right = Constants::WindowWidth;
		m_Scissor.top = 0;
		m_Scissor.bottom = Constants::WindowHeight;
	}

	return true;
}

void D3D12Wrapper::Terminate()
{
	// GPU処理の完了を待機
	m_Fence.Sync(m_pQueue.Get());

	// フェンスの破棄
	m_Fence.Term();

	// レンダーターゲットビューの破棄
	for (auto i = 0u; i < Constants::FrameCount; ++i)
	{
		m_RenderTarget[i].Term();
	}

	// 深度ステンシルバッファの破棄
	m_DepthTarget.Term();

	// コマンドリストの破棄
	m_CommandList.Term();

	for (auto i = 0; i < POOL_COUNT; ++i)
	{
		if (m_pPool[i] != nullptr)
		{
			m_pPool[i]->Release();
			m_pPool[i] = nullptr;
		}
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
		auto pTransform = m_pTransforms[m_FrameIndex]->GetPtr<Transform>();
		pTransform->World = Matrix::CreateRotationY(m_RotateAngle);
	}

	// コマンドの記録を開始
	auto pCmd = m_CommandList.Reset();

	// 書き込み用リソースバリアを設定
	DirectX::TransitionResource(
		pCmd,
		m_RenderTarget[m_FrameIndex].GetResource(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	// ディスクリプタを取得
	auto handleRTV = m_RenderTarget[m_FrameIndex].GetHandle();
	auto handleDSV = m_DepthTarget.GetHandle();

	// レンダーゲットの設定
	pCmd->OMSetRenderTargets(1, &handleRTV->HandleCPU, FALSE, &handleDSV->HandleCPU);

	// クリアカラーの設定
	float clearColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };

	// レンダーターゲットビューをクリア
	pCmd->ClearRenderTargetView(handleRTV->HandleCPU, clearColor, 0, nullptr);

	// 深度ステンシルビューをクリア
	pCmd->ClearDepthStencilView(handleDSV->HandleCPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 描画処理
	{
		ID3D12DescriptorHeap* const pHeaps[] = {
			m_pPool[POOL_TYPE_RES]->GetHeap()
		};

		pCmd->SetGraphicsRootSignature(m_pRootSignature.Get());
		pCmd->SetDescriptorHeaps(1, pHeaps);
		pCmd->SetGraphicsRootConstantBufferView(0, m_pTransforms[m_FrameIndex]->GetAddress());
		pCmd->SetGraphicsRootConstantBufferView(1, m_pLight->GetAddress());
		pCmd->SetPipelineState(m_pPSO.Get());
		pCmd->RSSetViewports(1, &m_Viewport);
		pCmd->RSSetScissorRects(1, &m_Scissor);

		for (size_t i = 0; i < m_pMeshes.size(); ++i)
		{
			// マテリアル番号を取得
			auto id = m_pMeshes[i]->GetMaterialId();

			// 定数バッファを設定
			pCmd->SetGraphicsRootConstantBufferView(2, m_Material.GetBufferAddress(i));

			// テクスチャを設定
			pCmd->SetGraphicsRootDescriptorTable(3, m_Material.GetTextureHandle(id, TU_DIFFUSE));
			pCmd->SetGraphicsRootDescriptorTable(4, m_Material.GetTextureHandle(id, TU_NORMAL));

			// 描画
			m_pMeshes[i]->Draw(pCmd);
		}
	}

	// 表示用リソースバリアを設定
	DirectX::TransitionResource(
		pCmd,
		m_RenderTarget[m_FrameIndex].GetResource(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	// コマンドの記録を終了
	pCmd->Close();

	// コマンドを実行
	ID3D12CommandList* ppCmdLists[] = { pCmd };
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
			ELOG("Error : File Not Found.");
			return false;
		}

		std::wstring dir = GetDirectoryPath(path.c_str());

		std::vector<ResMesh> resMesh;
		std::vector<ResMaterial> resMaterial;

		// メッシュリソースをロード
		if (!LoadMesh(path.c_str(), resMesh, resMaterial))
		{
			ELOG("Error : Load Mesh Failed. filepath = %ls", path.c_str());
			return false;
		}

		// メモリを予約
		m_pMeshes.reserve(resMesh.size());

		// メッシュを初期化
		for (size_t i = 0; i < resMesh.size(); ++i)
		{
			// メッシュ生成
			auto mesh = new(std::nothrow) Mesh();
			if (mesh == nullptr)
			{
				ELOG("Error : Out of Memory.");
				return false;
			}

			// 初期化処理
			if (!mesh->Init(m_pDevice.Get(), resMesh[i]))
			{
				ELOG("Error : Mesh::Init() Failed.");
				delete mesh;

				return false;
			}

			// メッシュを登録
			m_pMeshes.push_back(mesh);
		}

		// メモリを最適化
		m_pMeshes.shrink_to_fit();

		// マテリアル初期化
		if (!m_Material.Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(MaterialBuffer), resMaterial.size()))
		{
			ELOG("Error : Material::Init() Failed.");
			return false;
		}

		// リソースバッチを用意
		DirectX::ResourceUploadBatch batch(m_pDevice.Get());

		// バッチ開始
		batch.Begin();

		// テクスチャとマテリアルを設定
		for (size_t i = 0; i < resMaterial.size(); ++i)
		{
			auto ptr = m_Material.GetBufferPtr<MaterialBuffer>(i);
			ptr->BaseColor = resMaterial[i].Diffuse;
			ptr->Alpha = resMaterial[i].Alpha;
			ptr->Metalic = 0.5f;
			ptr->Shininess = resMaterial[i].Shininess;

			std::wstring path = dir + resMaterial[i].DiffuseMap;
			m_Material.SetTexture(i, TU_DIFFUSE, path, batch);

			path = dir + resMaterial[i].NormalMap;
			m_Material.SetTexture(i, TU_NORMAL, path, batch);
		}

		// バッチ終了
		auto future = batch.End(m_pQueue.Get());

		// バッチ完了を待機
		future.wait();
	}

	// ライトバッファの設定
	{
		auto pCB = new(std::nothrow) ConstantBuffer();
		if (pCB == nullptr)
		{
			ELOG("Error : Out of Memory.");
			return false;
		}

		if (!pCB->Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(LightBuffer)))
		{
			ELOG("Error : ConstantBuffer::Init() Failed.");
			return false;
		}

		auto ptr = pCB->GetPtr<LightBuffer>();
		ptr->LitghtPosition = Vector4(0.0f, 50.0f, 100.0f, 0.0f);
		ptr->LightColor = Color(1.0f, 1.0f, 1.0f, 0.0f);
		ptr->CameraPosition = Vector4(0.0f, 1.0f, 2.0f, 0.0f);

		m_pLight = pCB;
	}


	// ルートシグニチャの生成
	{
		auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
		flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
		flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		// ディスクリプタレンジの設定
		D3D12_DESCRIPTOR_RANGE range[2] = {};
		range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range[0].NumDescriptors = 1;
		range[0].BaseShaderRegister = 0;
		range[0].RegisterSpace = 0;
		range[0].OffsetInDescriptorsFromTableStart = 0;

		range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range[1].NumDescriptors = 1;
		range[1].BaseShaderRegister = 1;
		range[1].RegisterSpace = 0;
		range[1].OffsetInDescriptorsFromTableStart = 0;

		// ルートパラメータの設定
		D3D12_ROOT_PARAMETER param[5] = {};
		param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		param[0].Descriptor.ShaderRegister = 0;
		param[0].Descriptor.RegisterSpace = 0;
		param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		param[1].Descriptor.ShaderRegister = 1;
		param[1].Descriptor.RegisterSpace = 0;
		param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		param[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		param[2].Descriptor.ShaderRegister = 2;
		param[2].Descriptor.RegisterSpace = 0;
		param[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		param[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[3].DescriptorTable.NumDescriptorRanges = 1;
		param[3].DescriptorTable.pDescriptorRanges = &range[0];
		param[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		param[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[4].DescriptorTable.NumDescriptorRanges = 1;
		param[4].DescriptorTable.pDescriptorRanges = &range[1];
		param[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// スタティックサンプラーの設定
		auto sampler = DirectX::CommonStates::StaticLinearWrap(0, D3D12_SHADER_VISIBILITY_PIXEL);

		// ルートシグニチャの設定
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = _countof(param);
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
			ELOG("Error : Root Signature Create Failed. retcode = 0x%x", hr);
			return false;
		}
	}

	// パイプラインステートの生成
	{
		std::wstring vsPath;
		std::wstring psPath;

		// 頂点シェーダーを検索
		if (!SearchFilePath(L"SimpleTexVS.cso", vsPath))
		{
			ELOG("Error : Vertex Shader Not Found.");
			return false;
		}

		// ピクセルシェーダーを検索
		if (!SearchFilePath(L"SimpleTexPS.cso", psPath))
		{
			ELOG("Error : Pixel Shader Not Found.");
			return false;
		}

		ComPtr<ID3DBlob> pVSBlob;
		ComPtr<ID3DBlob> pPSBlob;

		// 頂点シェーダー読み込み
		auto hr = D3DReadFileToBlob(vsPath.c_str(), pVSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			ELOG("Error : D3DReadFiledToBlob() Failed. path = %ls", vsPath.c_str());
			return false;
		}

		// ピクセルシェーダー読み込み
		hr = D3DReadFileToBlob(psPath.c_str(), pPSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			ELOG("Error : D3DReadFileToBlob() Failed. path = %ls", psPath.c_str());
			return false;
		}

		// パイプラインステートの設定
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.InputLayout = MeshVertex::InputLayout;
		desc.pRootSignature = m_pRootSignature.Get();
		desc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
		desc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
		desc.RasterizerState = DirectX::CommonStates::CullNone;
		desc.BlendState = DirectX::CommonStates::Opaque;
		desc.DepthStencilState = DirectX::CommonStates::DepthDefault;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = m_RenderTarget[0].GetViewDesc().Format;
		desc.DSVFormat = m_DepthTarget.GetViewDesc().Format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		// パイプラインステートを生成
		hr = m_pDevice->CreateGraphicsPipelineState(
			&desc,
			IID_PPV_ARGS(m_pPSO.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
			return false;
		}
	}

	// 変換行列用の定数バッファの生成
	{
		m_pTransforms.reserve(Constants::FrameCount);

		for (auto i = 0u; i < Constants::FrameCount; ++i)
		{
			auto pCB = new(std::nothrow) ConstantBuffer();
			if (pCB == nullptr)
			{
				ELOG("Error : Out of memory.");
				return false;
			}

			// 定数バッファ初期化
			if (!pCB->Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(Transform) * 2))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");
				return false;
			}

			// カメラ設定
			auto eyePos = Vector3(0.0f, 1.0f, 2.0f);
			auto targetPos = Vector3::Zero;
			auto upward = Vector3::UnitY;

			// 垂直画角とアスペクト比の設定.
			auto fovY = DirectX::XMConvertToRadians(37.5f);
			auto aspect = static_cast<float>(Constants::WindowWidth) / static_cast<float>(Constants::WindowHeight);

			// 変換行列を設定.
			auto ptr = pCB->GetPtr<Transform>();
			ptr->World = Matrix::Identity;
			ptr->View = Matrix::CreateLookAt(eyePos, targetPos, upward);
			ptr->Proj = Matrix::CreatePerspectiveFieldOfView(fovY, aspect, 1.0f, 1000.0f);

			m_pTransforms.push_back(pCB);
		}

		m_RotateAngle = 0.0f;
	}

	return true;
}

void D3D12Wrapper::ReleaseGraphicsResources()
{
	// メッシュの破棄
	for (size_t i = 0; i < m_pMeshes.size(); ++i)
	{
		SafeTerm(m_pMeshes[i]);
	}

	m_pMeshes.clear();
	m_pMeshes.shrink_to_fit();

	// マテリアルの破棄
	m_Material.Term();

	// ライト破棄
	SafeDelete(m_pLight);

	// 変換バッファの破棄
	for (size_t i = 0; i < m_pTransforms.size(); ++i)
	{
		SafeTerm(m_pTransforms[i]);
	}

	m_pTransforms.clear();
	m_pTransforms.shrink_to_fit();
}

void D3D12Wrapper::CheckSupportHDR()
{
	if (m_pSwapChain == nullptr || m_pFactory == nullptr || m_pDevice == nullptr)
	{
		return;
	}

	HRESULT hr = S_OK;

	// ウィンドウ領域を取得
	RECT rect = {};
	GetWindowRect(m_hWnd, &rect);

	if (m_pFactory->IsCurrent() == false)
	{
		m_pFactory.Reset();
		hr = CreateDXGIFactory2(0, IID_PPV_ARGS(m_pFactory.GetAddressOf()));
		if (FAILED(hr))
		{
			return;
		}
	}

	ComPtr<IDXGIAdapter1> pAdapter;
	hr = m_pFactory->EnumAdapters1(0, pAdapter.GetAddressOf());
	if (FAILED(hr))
	{
		return;
	}

	UINT i = 0;
	ComPtr<IDXGIOutput> pCurrentOutput;
	ComPtr<IDXGIOutput> pBestOutput;
	int bestIntersectArea = -1;

	// 各ディスプレイを調べる
	while (pAdapter->EnumOutputs(i, &pCurrentOutput) != DXGI_ERROR_NOT_FOUND)
	{
		auto ax1 = rect.left;
		auto ay1 = rect.top;
		auto ax2 = rect.right;
		auto ay2 = rect.bottom;

		// ディスプレイの設定を取得
		DXGI_OUTPUT_DESC desc;
		hr = pCurrentOutput->GetDesc(&desc);
		if (FAILED(hr))
		{
			return;
		}

		auto bx1 = desc.DesktopCoordinates.left;
		auto by1 = desc.DesktopCoordinates.top;
		auto bx2 = desc.DesktopCoordinates.right;
		auto by2 = desc.DesktopCoordinates.bottom;

		// 領域が一致するかどうか調べる
		int intersectArea = ComputeIntersectionArea(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2);
		if (intersectArea > bestIntersectArea)
		{
			pBestOutput = pCurrentOutput;
			bestIntersectArea = intersectArea;
		}

		i++;
	}

	// 一番適しているディスプレイ
	ComPtr<IDXGIOutput6> pOutput6;
	hr = pBestOutput.As(&pOutput6);
	if (FAILED(hr))
	{
		return;
	}

	// 出力設定を取得
	DXGI_OUTPUT_DESC1 desc1;
	hr = pOutput6->GetDesc1(&desc1);
	if (FAILED(hr))
	{
		return;
	}

	// 色空間が ITU-R BT.2100 PQ をサポートしているかどうかチェック
	m_SupportHDR = (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
	m_MaxLuminance = desc1.MaxLuminance;
	m_MinLuminance = desc1.MinLuminance;
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


void D3D12Wrapper::Present(uint32_t interval)
{
	// 画面に表示
	m_pSwapChain->Present(interval, 0);

	// 完了待ち
	m_Fence.Wait(m_pQueue.Get(), INFINITE);

	// フレーム番号を更新
	m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}
