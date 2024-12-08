#include "D3D12Wrapper.h"

#include <cassert>
#include <ResourceUploadBatch.h>
#include <DDSTextureLoader.h>
#include <CommonStates.h>
#include <VertexTypes.h>
#include <DirectXHelpers.h>
#include <SimpleMath.h>
#include <algorithm>

#include "InputSystem.h"
#include "FileUtil.h" 
#include "Logger.h"

using namespace DirectX::SimpleMath;

namespace
{
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

	enum COLOR_SPACE_TYPE
	{
		COLOR_SPACE_BT709,      // ITU-R BT.709
		COLOR_SPACE_BT2100_PQ,  // ITU-R BT.2100 PQ System
	};

	enum TONEMAP_TYPE
	{
		TONEMAP_NONE = 0,   // トーンマップなし
		TONEMAP_REINHARD,   // Reinhardトーンマップ
		TONEMAP_GT,         // GTトーンマップ
	};

	struct alignas(256) CbTonemap
	{
		int     Type;               // トーンマップタイプ
		int     ColorSpace;         // 出力色空間
		float   BaseLuminance;      // 基準輝度値[nit]
		float   MaxLuminance;       // 最大輝度値[nit]
	};

	struct alignas(256) CbMesh
	{
		Matrix World; // ワールド行列
	};

	struct alignas(256) CbTransform
	{
		Matrix View; // ビュー行列
		Matrix Proj; // 射影行列
	};

	struct alignas(256) CbDirectionalLight
	{
		Vector3 LightColor;			// ライトの色
		float   LightIntensity;		// ライトの強度
		Vector3 LightForward;		// ライトの照射方向
	};

	struct alignas(256) CbLight
	{
		Vector3 LightPosition;		// ライトの位置
		float   LightInvSqrRadius;	// ライトの逆二乗半径
		Vector3 LightColor;			// ライトの色
		float   LightIntensity;		// ライトの強度
		Vector3 LightForward;		// ライトの照射方向
		float   LightAngleScale;	// ライトの照射角度スケール( 1.0f / (cosInner - cosOuter) )
		float	LightAngleOffset;	// ライトの照射角度オフセット( -cosOuter * LightAngleScale )
		int		LightType;			// ライトの種類
		float   Padding[2];			// パディング
	};

	struct alignas(256) CbCamera
	{
		Vector3 CameraPosition;		// カメラの位置
	};

	struct alignas(256) CbMaterial
	{
		Vector3 BaseColor;		// 基本色
		float	Alpha;			// アルファ値
		float   Roughness;		// 粗さ
		float   Metallic;		// 金属度
	};

	UINT16 inline GetChromaticityCoord(double value)
	{
		return static_cast<UINT16>(value * 50000.0);
	}

	CbLight ComputePointLight(const Vector3& pos, float radius, const Vector3& color, float intensity)
	{
		CbLight result;
		result.LightPosition = pos;
		result.LightInvSqrRadius = 1.0f / (radius * radius);
		result.LightColor = color;
		result.LightIntensity = intensity;

		return result;
	}

	CbLight ComputeSpotLight
	(
		int lightType,
		const Vector3& dir,
		const Vector3& pos,
		float radius,
		const Vector3& color,
		float intensity,
		float innerAngle,
		float outerAngle
	)
	{
		auto cosInnerAngle = cosf(innerAngle);
		auto cosOuterAngle = cosf(outerAngle);

		CbLight result;
		result.LightPosition = pos;
		result.LightInvSqrRadius = 1.0f / (radius * radius);
		result.LightColor = color;
		result.LightIntensity = intensity;
		result.LightForward = dir;
		result.LightAngleScale = 1.0f / DirectX::XMMax(0.001f, (cosInnerAngle - cosOuterAngle));
		result.LightAngleOffset = -cosOuterAngle * result.LightAngleScale;
		result.LightType = lightType;

		return result;
	}

	Vector3 CalcLightColor(float time)
	{
		auto c = fmodf(time, 3.0f);
		auto result = Vector3(0.25, 0.25, 0.25);

		if (c < 1.0f)
		{
			result.x += 1.0f - c;
			result.y += c;
		}
		else if (c < 2.0f)
		{
			c -= 1.0f;
			result.y += 1.0f - c;
			result.z += c;
		}
		else
		{
			c -= 2.0f;
			result.z += 1.0f - c;
			result.x += c;
		}

		return result;
	}
}

D3D12Wrapper::D3D12Wrapper()
	: m_hWnd(nullptr)
	, m_pFactory(nullptr)
	, m_pDevice(nullptr)
	, m_pQueue(nullptr)
	, m_pSwapChain(nullptr)
	, m_FrameIndex(0)
	, m_BackBufferFormat(DXGI_FORMAT_R10G10B10A2_UNORM)
	, m_TonemapType(TONEMAP_NONE)
	, m_ColorSpace(COLOR_SPACE_BT709)
	, m_BaseLuminance(100.0f)
	, m_MaxLuminance(100.0f)
	, m_Exposure(1.0f)
	, m_LightType(0)
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
		desc.BufferDesc.Format = m_BackBufferFormat;
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

		// IDXGISwapChain4 を取得
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
		if (!m_DepthTarget.Init(m_pDevice.Get(), m_pPool[POOL_TYPE_DSV], nullptr, Constants::WindowWidth, Constants::WindowHeight, DXGI_FORMAT_D32_FLOAT, 1.0, 0))
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
	// コマンドの記録を開始
	auto pCmd = m_CommandList.Reset();

	ID3D12DescriptorHeap* const pHeaps[] = {
		m_pPool[POOL_TYPE_RES]->GetHeap()
	};

	pCmd->SetDescriptorHeaps(1, pHeaps);

	{
		// ディスクリプタを取得
		auto handleRTV = m_SceneColorTarget.GetHandleRTV();
		auto handleDSV = m_SceneDepthTarget.GetHandleDSV();

		// 書き込み用リソースバリアを設定
		DirectX::TransitionResource(
			pCmd,
			m_SceneColorTarget.GetResource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		// レンダーゲットの設定
		pCmd->OMSetRenderTargets(1, &handleRTV->HandleCPU, FALSE, &handleDSV->HandleCPU);

		// レンダーターゲットをクリア.
		m_SceneColorTarget.ClearView(pCmd);
		m_SceneDepthTarget.ClearView(pCmd);

		DrawScene(pCmd);

		// 読み込み用リソースバリアを設定
		DirectX::TransitionResource(
			pCmd,
			m_SceneColorTarget.GetResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}


	// フレームバッファに描画
	{
		// 書き込み用リソースバリアを設定
		DirectX::TransitionResource(
			pCmd,
			m_RenderTarget[m_FrameIndex].GetResource(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		// ディスクリプタを取得
		auto handleRTV = m_RenderTarget[m_FrameIndex].GetHandleRTV();
		auto handleDSV = m_DepthTarget.GetHandleDSV();

		// レンダーターゲットの設定
		pCmd->OMSetRenderTargets(1, &handleRTV->HandleCPU, FALSE, &handleDSV->HandleCPU);

		// レンダーターゲットをクリア
		m_RenderTarget[m_FrameIndex].ClearView(pCmd);
		m_DepthTarget.ClearView(pCmd);

		// トーンマップを適用
		DrawTonemap(pCmd);

		// 表示用リソースバリアを設定
		DirectX::TransitionResource(
			pCmd,
			m_RenderTarget[m_FrameIndex].GetResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
	}

	// コマンドの記録を終了
	pCmd->Close();

	// コマンドを実行
	ID3D12CommandList* ppCmdLists[] = { pCmd };
	m_pQueue->ExecuteCommandLists(1, ppCmdLists);

	// 画面に表示
	Present(1);
}

void D3D12Wrapper::ProcessInput(const InputState& state)
{
	// HDRモード
	if (state.keyboard.GetKeyState('H') == ButtonState::Pressed)
	{
		ChangeDisplayMode(true);
	}

	// SDRモード
	if (state.keyboard.GetKeyState('S') == ButtonState::Pressed)
	{
		ChangeDisplayMode(false);
	}

	// トーンマップなし
	if (state.keyboard.GetKeyState('N') == ButtonState::Pressed)
	{
		m_TonemapType = TONEMAP_NONE;
	}

	// Reinhardトーンマップ
	if (state.keyboard.GetKeyState('R') == ButtonState::Pressed)
	{
		m_TonemapType = TONEMAP_REINHARD;
	}

	// GTトーンマップ
	if (state.keyboard.GetKeyState('G') == ButtonState::Pressed)
	{
		m_TonemapType = TONEMAP_GT;
	}

	// スポットライトの計算手法の切り替え
	if (state.keyboard.GetKeyState('L') == ButtonState::Pressed)
	{
		m_LightType = (m_LightType + 1) % 3;

		// 距離減衰なし
		if (m_LightType == 0)
		{
			printf_s("SpotLight : Default\n");
		}
		// [Karis 2013] Brian Karis, "Real Shading in Unreal Engine4", SIGGRAPH 2013 Course: Physically Based Shading in Theory and Parctice.
		else if (m_LightType == 1)
		{
			printf_s("SpotLight : [Karis 2013]\n");
		}
		// [Lagarde, Rousiers 2014] Sébastien Lagarde, Charles de Reousiers, "Moving Frostbite to Physically Based Rendering 2.0", SIGGRAPH 2014 Course: Physically Based Shading in Theory and Practice.
		else if (m_LightType == 2)
		{
			printf_s("SpotLight : [Lagarde, Rousiers 2014]\n");
		}
	}
}

bool D3D12Wrapper::InitializeGraphicsPipeline()
{
	// メッシュをロード
	{
		std::wstring path;
		if (!SearchFilePath(L"Assets/material_test/material_test.obj", path))
			//if (!SearchFilePath(L"Assets/sponza/glTF/Sponza_modified.gltf", path))
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
		if (!m_Material.Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbMaterial), resMaterial.size()))
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
			/*std::wstring path = dir + resMaterial[i].BaseColorMap;
			m_Material.SetTexture(i, TU_BASE_COLOR, path, batch);

			path = dir + resMaterial[i].NormalMap;
			m_Material.SetTexture(i, TU_NORMAL, path, batch);

			path = dir + resMaterial[i].MetallicMap;
			m_Material.SetTexture(i, TU_METALLIC, path, batch);

			path = dir + resMaterial[i].RoughnessMap;
			m_Material.SetTexture(i, TU_ROUGHNESS, path, batch);*/
		}

		{
			/* ここではマテリアルが決め打ちであることを前提にハードコーディングしています. */
			m_Material.SetTexture(0, TU_BASE_COLOR, dir + L"wall_bc.dds", batch);
			m_Material.SetTexture(0, TU_METALLIC, dir + L"wall_m.dds", batch);
			m_Material.SetTexture(0, TU_ROUGHNESS, dir + L"wall_r.dds", batch);
			m_Material.SetTexture(0, TU_NORMAL, dir + L"wall_n.dds", batch);

			m_Material.SetTexture(1, TU_BASE_COLOR, dir + L"matball_bc.dds", batch);
			m_Material.SetTexture(1, TU_METALLIC, dir + L"matball_m.dds", batch);
			m_Material.SetTexture(1, TU_ROUGHNESS, dir + L"matball_r.dds", batch);
			m_Material.SetTexture(1, TU_NORMAL, dir + L"matball_n.dds", batch);
		}

		// バッチ終了
		auto future = batch.End(m_pQueue.Get());

		// バッチ完了を待機
		future.wait();
	}

	// ライトバッファの設定
	{
		// ディレクショナルライト
		for (auto i = 0; i < Constants::FrameCount; ++i)
		{
			if (!m_DirectionalLightCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbDirectionalLight)))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");
				return false;
			}
		}

		for (auto i = 0; i < Constants::FrameCount; ++i)
		{
			if (!m_LightCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbLight)))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");
				return false;
			}
		}
	}

	// カメラバッファの設定
	{
		for (auto i = 0; i < Constants::FrameCount; ++i)
		{
			if (!m_CameraCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbCamera)))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");
				return false;
			}
		}
	}

	// シーン用カラーターゲットの生成
	{
		float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };

		if (!m_SceneColorTarget.Init(
			m_pDevice.Get(),
			m_pPool[POOL_TYPE_RTV],
			m_pPool[POOL_TYPE_RES],
			Constants::WindowWidth,
			Constants::WindowHeight,
			DXGI_FORMAT_R10G10B10A2_UNORM,
			clearColor))
		{
			ELOG("Error : ColorTarget::Init() Failed.");
			return false;
		}
	}

	// シーン用深度ステンシルバッファの生成
	{
		if (!m_SceneDepthTarget.Init(
			m_pDevice.Get(),
			m_pPool[POOL_TYPE_DSV],
			nullptr,
			Constants::WindowWidth,
			Constants::WindowHeight,
			DXGI_FORMAT_D32_FLOAT,
			1.0,
			0))
		{
			ELOG("Error : DepthTarget::Init() Failed.");
			return false;
		}
	}

	// シーン用ルートシグネチャの生成
	{
		RootSignature::Desc desc;
		desc.Begin(9)
			.SetCBV(ShaderStage::VS, 0, 0) // 変換行列
			.SetCBV(ShaderStage::VS, 1, 1) // メッシュのワールド行列
			.SetCBV(ShaderStage::PS, 8, 0) // ディレクショナルライト
			.SetCBV(ShaderStage::PS, 2, 1) // ライト
			.SetCBV(ShaderStage::PS, 3, 2) // カメラ
			.SetSRV(ShaderStage::PS, 4, 0) // ベースカラーマップ
			.SetSRV(ShaderStage::PS, 5, 1) // 金属度マップ
			.SetSRV(ShaderStage::PS, 6, 2) // 粗さマップ
			.SetSRV(ShaderStage::PS, 7, 3) // 法線マップ
			.AddStaticSmp(ShaderStage::PS, 0, SamplerState::LinearClamp) // ベースカラーマップ
			.AddStaticSmp(ShaderStage::PS, 1, SamplerState::LinearClamp) // 金属度マップ
			.AddStaticSmp(ShaderStage::PS, 2, SamplerState::LinearClamp) // 粗さマップ
			.AddStaticSmp(ShaderStage::PS, 3, SamplerState::LinearClamp) // 法線マップ
			.AllowIL()
			.End();

		if (!m_SceneRootSignature.Init(m_pDevice.Get(), desc.GetDesc()))
		{
			ELOG("Error : RootSignature::Init() Failed.");
			return false;
		}
	}

	// シーン用パイプラインステートの生成
	{
		std::wstring vsPath;
		std::wstring psPath;

		// 頂点シェーダーを検索
		if (!SearchFilePath(L"BasicVS.cso", vsPath))
		{
			ELOG("Error : Vertex Shader Not Found.");
			return false;
		}

		// ピクセルシェーダーを検索
		if (!SearchFilePath(L"BasicPS.cso", psPath))
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

		D3D12_INPUT_ELEMENT_DESC elements[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// パイプラインステートの設定
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.InputLayout = { elements, 4 };
		desc.pRootSignature = m_SceneRootSignature.GetPtr();
		desc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
		desc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
		desc.RasterizerState = DirectX::CommonStates::CullNone;
		desc.BlendState = DirectX::CommonStates::Opaque;
		desc.DepthStencilState = DirectX::CommonStates::DepthDefault;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = m_SceneColorTarget.GetRTVDesc().Format;
		desc.DSVFormat = m_SceneDepthTarget.GetDSVDesc().Format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		// パイプラインステートを生成
		hr = m_pDevice->CreateGraphicsPipelineState(
			&desc,
			IID_PPV_ARGS(m_pScenePSO.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
			return false;
		}
	}

	// トーンマップ用ルートシグネチャの生成
	{
		RootSignature::Desc desc;
		desc.Begin(2)
			.SetCBV(ShaderStage::PS, 0, 0)
			.SetSRV(ShaderStage::PS, 1, 0)
			.AddStaticSmp(ShaderStage::PS, 0, SamplerState::LinearWrap)
			.AllowIL()
			.End();

		if (!m_TonemapRootSignature.Init(m_pDevice.Get(), desc.GetDesc()))
		{
			ELOG("Error : RootSignature::Init() Failed.");
			return false;
		}
	}

	// トーンマップ用パイプラインステートの生成
	{
		std::wstring vsPath;
		std::wstring psPath;

		// 頂点シェーダを検索
		if (!SearchFilePath(L"TonemapVS.cso", vsPath))
		{
			ELOG("Error : Vertex Shader Not Found.");
			return false;
		}

		// ピクセルシェーダを検索
		if (!SearchFilePath(L"TonemapPS.cso", psPath))
		{
			ELOG("Error : Pixel Shader Node Found.");
			return false;
		}

		ComPtr<ID3DBlob> pVSBlob;
		ComPtr<ID3DBlob> pPSBlob;

		// 頂点シェーダを読み込む
		auto hr = D3DReadFileToBlob(vsPath.c_str(), pVSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			ELOG("Error : D3DReadFiledToBlob() Failed. path = %ls", vsPath.c_str());
			return false;
		}

		// ピクセルシェーダを読み込む
		hr = D3DReadFileToBlob(psPath.c_str(), pPSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			ELOG("Error : D3DReadFileToBlob() Failed. path = %ls", psPath.c_str());
			return false;
		}

		D3D12_INPUT_ELEMENT_DESC elements[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// グラフィックスパイプラインステートを設定.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.InputLayout = { elements, 2 };
		desc.pRootSignature = m_TonemapRootSignature.GetPtr();
		desc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
		desc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
		desc.RasterizerState = DirectX::CommonStates::CullNone;
		desc.BlendState = DirectX::CommonStates::Opaque;
		desc.DepthStencilState = DirectX::CommonStates::DepthDefault;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = m_RenderTarget[0].GetRTVDesc().Format;
		desc.DSVFormat = m_DepthTarget.GetDSVDesc().Format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		// パイプラインステートを生成.
		hr = m_pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pTonemapPSO.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
			return false;
		}
	}

	// 頂点バッファの生成.
	{
		struct Vertex
		{
			float px;
			float py;

			float tx;
			float ty;
		};

		if (!m_QuadVB.Init<Vertex>(m_pDevice.Get(), 3))
		{
			ELOG("Error : VertexBuffer::Init() Failed.");
			return false;
		}

		auto ptr = m_QuadVB.Map<Vertex>();
		assert(ptr != nullptr);
		ptr[0].px = -1.0f;  ptr[0].py = 1.0f;  ptr[0].tx = 0.0f;   ptr[0].ty = -1.0f;
		ptr[1].px = 3.0f;  ptr[1].py = 1.0f;  ptr[1].tx = 2.0f;   ptr[1].ty = -1.0f;
		ptr[2].px = -1.0f;  ptr[2].py = -3.0f;  ptr[2].tx = 0.0f;   ptr[2].ty = 1.0f;
		m_QuadVB.Unmap();
	}

	// 壁用頂点バッファの生成.
	{
		struct BasicVertex
		{
			Vector3 Position;
			Vector3 Normal;
			Vector2 TexCoord;
			Vector3 Tangent;
		};

		if (!m_WallVB.Init<BasicVertex>(m_pDevice.Get(), 6))
		{
			ELOG("Error : VertexBuffer::Init() Failed.");
			return false;
		}

		auto size = 10.0f;
		auto ptr = m_WallVB.Map<BasicVertex>();
		assert(ptr != nullptr);

		ptr[0].Position = Vector3(-size, size, 0.0f);
		ptr[0].Normal = Vector3(0.0f, 0.0f, 1.0f);
		ptr[0].TexCoord = Vector2(0.0f, 1.0f);
		ptr[0].Tangent = Vector3(1.0f, 0.0f, 0.0f);

		ptr[1].Position = Vector3(size, size, 0.0f);
		ptr[1].Normal = Vector3(0.0f, 0.0f, 1.0f);
		ptr[1].TexCoord = Vector2(1.0f, 1.0f);
		ptr[1].Tangent = Vector3(1.0f, 0.0f, 0.0f);

		ptr[2].Position = Vector3(size, -size, 0.0f);
		ptr[2].Normal = Vector3(0.0f, 0.0f, 1.0f);
		ptr[2].TexCoord = Vector2(1.0f, 0.0f);
		ptr[2].Tangent = Vector3(1.0f, 0.0f, 0.0f);

		ptr[3].Position = Vector3(-size, size, 0.0f);
		ptr[3].Normal = Vector3(0.0f, 0.0f, 1.0f);
		ptr[3].TexCoord = Vector2(0.0f, 1.0f);
		ptr[3].Tangent = Vector3(1.0f, 0.0f, 0.0f);

		ptr[4].Position = Vector3(size, -size, 0.0f);
		ptr[4].Normal = Vector3(0.0f, 0.0f, 1.0f);
		ptr[4].TexCoord = Vector2(1.0f, 0.0f);
		ptr[4].Tangent = Vector3(1.0f, 0.0f, 0.0f);

		ptr[5].Position = Vector3(-size, -size, 0.0f);
		ptr[5].Normal = Vector3(0.0f, 0.0f, 1.0f);
		ptr[5].TexCoord = Vector2(0.0f, 0.0f);
		ptr[5].Tangent = Vector3(1.0f, 0.0f, 0.0f);

		m_WallVB.Unmap();
	}

	for (auto i = 0; i < Constants::FrameCount; ++i)
	{
		if (!m_TonemapCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbTonemap)))
		{
			ELOG("Error : ConstantBuffer::Init() Failed.");
			return false;
		}
	}

	// 変換行列用の定数バッファの生成.
	{
		for (auto i = 0u; i < Constants::FrameCount; ++i)
		{
			// 定数バッファ初期化.
			if (!m_TransformCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbTransform)))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");
				return false;
			}

			// カメラ設定.
			auto eyePos = Vector3(0.0f, 1.0f, 2.0f);
			auto targetPos = Vector3::Zero;
			auto upward = Vector3::UnitY;

			// 垂直画角とアスペクト比の設定.
			auto fovY = DirectX::XMConvertToRadians(37.5f);
			auto aspect = static_cast<float>(Constants::WindowWidth) / static_cast<float>(Constants::WindowHeight);

			// 変換行列を設定.
			auto ptr = m_TransformCB[i].GetPtr<CbTransform>();
			ptr->View = Matrix::CreateLookAt(eyePos, targetPos, upward);
			ptr->Proj = Matrix::CreatePerspectiveFieldOfView(fovY, aspect, 1.0f, 1000.0f);
		}

		m_RotateAngle = DirectX::XMConvertToRadians(-60.0f);
	}

	// メッシュ用バッファの生成.
	{
		for (auto i = 0; i < Constants::FrameCount; ++i)
		{
			if (!m_MeshCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbMesh)))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");
				return false;
			}

			auto ptr = m_MeshCB[i].GetPtr<CbMesh>();
			ptr->World = Matrix::Identity;
		}
	}

	// 開始時間を記録
	m_StartTime = std::chrono::system_clock::now();

	return true;
}

void D3D12Wrapper::ReleaseGraphicsResources()
{
	m_QuadVB.Term();

	for (auto i = 0; i < Constants::FrameCount; ++i)
	{
		m_TonemapCB[i].Term();
		m_DirectionalLightCB[i].Term();
		m_LightCB[i].Term();
		m_CameraCB[i].Term();
		m_TransformCB[i].Term();
	}

	// メッシュの破棄
	for (size_t i = 0; i < m_pMeshes.size(); ++i)
	{
		SafeTerm(m_pMeshes[i]);
	}

	m_pMeshes.clear();
	m_pMeshes.shrink_to_fit();

	// マテリアルの破棄
	m_Material.Term();

	for (auto i = 0; i < Constants::FrameCount; ++i)
	{
		m_MeshCB[i].Term();
	}

	m_SceneColorTarget.Term();
	m_SceneDepthTarget.Term();

	m_pScenePSO.Reset();
	m_SceneRootSignature.Term();

	m_pTonemapPSO.Reset();
	m_TonemapRootSignature.Term();
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
	m_MaxDisplayLuminance = desc1.MaxLuminance;
	m_MinDisplayLuminance = desc1.MinLuminance;
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

void D3D12Wrapper::ChangeDisplayMode(bool hdr)
{
	if (hdr)
	{
		if (!m_SupportHDR)
		{
			MessageBox(
				nullptr,
				TEXT("HDRがサポートされていないディスプレイです。"),
				TEXT("HDR非サポート"),
				MB_OK | MB_ICONINFORMATION);
			ELOG("Error : Display not support HDR.");
			return;
		}

		auto hr = m_pSwapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
		if (FAILED(hr))
		{
			MessageBox(
				nullptr,
				TEXT("ITU-R BT.2100 PQ Systemの色域設定に失敗しました。"),
				TEXT("色域設定失敗"),
				MB_OK | MB_ICONERROR);
			ELOG("Error : IDXGISwapChain::SetColorSpace1() Failed.");
			return;
		}

		DXGI_HDR_METADATA_HDR10 metaData = {};

		// ITU-R BT.2100の原刺激と白色点を設定
		metaData.RedPrimary[0] = GetChromaticityCoord(0.708);
		metaData.RedPrimary[1] = GetChromaticityCoord(0.292);
		metaData.BluePrimary[0] = GetChromaticityCoord(0.170);
		metaData.BluePrimary[1] = GetChromaticityCoord(0.797);
		metaData.GreenPrimary[0] = GetChromaticityCoord(0.131);
		metaData.GreenPrimary[1] = GetChromaticityCoord(0.046);
		metaData.WhitePoint[0] = GetChromaticityCoord(0.3127);
		metaData.WhitePoint[1] = GetChromaticityCoord(0.3290);

		// ディスプレイがサポートすると最大輝度値と最小輝度値を設定.
		metaData.MaxMasteringLuminance = UINT(m_MaxDisplayLuminance * 10000);
		metaData.MinMasteringLuminance = UINT(m_MinDisplayLuminance * 0.001);

		// 最大値を 2000 [nit]に設定
		metaData.MaxContentLightLevel = 2000;

		hr = m_pSwapChain->SetHDRMetaData(
			DXGI_HDR_METADATA_TYPE_HDR10,
			sizeof(DXGI_HDR_METADATA_HDR10),
			&metaData);
		if (FAILED(hr))
		{
			ELOG("Error : IDXGISwapChain::SetHDRMetaData() Failed.");
		}

		m_BaseLuminance = 100.0f;
		m_MaxLuminance = m_MaxDisplayLuminance;

		// 成功したことを知らせるダイアログを出す
		std::string message;
		message += "HDRディスプレイ用に設定を変更しました\n\n";
		message += "色空間：ITU-R BT.2100 PQ\n";
		message += "最大輝度値：";
		message += std::to_string(m_MaxDisplayLuminance);
		message += " [nit]\n";
		message += "最小輝度値：";
		message += std::to_string(m_MinDisplayLuminance);
		message += " [nit]\n";

		MessageBoxA(nullptr,
					message.c_str(),
					"HDR設定成功",
					MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		auto hr = m_pSwapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
		if (FAILED(hr))
		{
			MessageBox(
				nullptr,
				TEXT("ITU-R BT.709の色域設定に失敗しました。"),
				TEXT("色域設定失敗"),
				MB_OK | MB_ICONERROR);
			ELOG("Error : IDXGISwapChain::SetColorSpace1() Failed.");
			return;
		}

		DXGI_HDR_METADATA_HDR10 metaData = {};

		// ITU-R BT.709の原刺激と白色点を設定
		metaData.RedPrimary[0] = GetChromaticityCoord(0.640);
		metaData.RedPrimary[1] = GetChromaticityCoord(0.330);
		metaData.BluePrimary[0] = GetChromaticityCoord(0.300);
		metaData.BluePrimary[1] = GetChromaticityCoord(0.600);
		metaData.GreenPrimary[0] = GetChromaticityCoord(0.150);
		metaData.GreenPrimary[1] = GetChromaticityCoord(0.060);
		metaData.WhitePoint[0] = GetChromaticityCoord(0.3127);
		metaData.WhitePoint[1] = GetChromaticityCoord(0.3290);

		// ディスプレイがサポートすると最大輝度値と最小輝度値を設定.
		metaData.MaxMasteringLuminance = UINT(m_MaxDisplayLuminance * 10000);
		metaData.MinMasteringLuminance = UINT(m_MinDisplayLuminance * 0.001);

		// 最大値を 100[nit] に設定
		metaData.MaxContentLightLevel = 100;

		hr = m_pSwapChain->SetHDRMetaData(
			DXGI_HDR_METADATA_TYPE_HDR10,
			sizeof(DXGI_HDR_METADATA_HDR10),
			&metaData);
		if (FAILED(hr))
		{
			ELOG("Error : IDXGISwapChain::SetHDRMetaData() Failed.");
		}

		m_BaseLuminance = 100.0f;
		m_MaxLuminance = 100.0f;

		// 成功したことを知らせるダイアログを出す.
		std::string message;
		message += "SDRディスプレイ用に設定を変更しました\n\n";
		message += "色空間：ITU-R BT.709\n";
		message += "最大輝度値：";
		message += std::to_string(m_MaxDisplayLuminance);
		message += " [nit]\n";
		message += "最小輝度値：";
		message += std::to_string(m_MinDisplayLuminance);
		message += " [nit]\n";
		MessageBoxA(nullptr,
					message.c_str(),
					"SDR設定成功",
					MB_OK | MB_ICONINFORMATION);
	}
}

void D3D12Wrapper::DrawScene(ID3D12GraphicsCommandList* pCmdList)
{
	auto cameraPos = Vector3(1.0f, 5.5f, 3.0f);

	auto currTime = std::chrono::system_clock::now();
	auto dt = float(std::chrono::duration_cast<std::chrono::milliseconds>(currTime - m_StartTime).count()) / 1000.0f;
	auto lightColor = CalcLightColor(dt * 0.25f);

	// ディレクショナルライトの更新
	{
		auto matrix = Matrix::CreateRotationY(m_RotateAngle);

		auto ptr = m_DirectionalLightCB[m_FrameIndex].GetPtr<CbDirectionalLight>();
		ptr->LightColor = Vector3(1.0f, 1.0f, 1.0f);
		ptr->LightForward = Vector3::TransformNormal(Vector3(0.0f, 1.0f, 1.0f), matrix);
		ptr->LightIntensity = 2.0f;

		m_RotateAngle += 0.01f;
	}

	// ライトバッファの更新
	{
		auto pos = Vector3(-1.5f, 0.0f, 1.5f);
		auto dir = Vector3(1.0f, -0.1f, -1.0f);
		dir.Normalize();

		auto ptr = m_LightCB[m_FrameIndex].GetPtr<CbLight>();
		*ptr = ComputeSpotLight(
			m_LightType,
			dir,
			pos,
			3.0f,
			lightColor,
			810.0f,
			DirectX::XMConvertToRadians(5.0f),
			DirectX::XMConvertToRadians(20.0f));
	}

	// カメラバッファの更新
	{
		auto ptr = m_CameraCB[m_FrameIndex].GetPtr<CbCamera>();
		ptr->CameraPosition = cameraPos;
	}

	// メッシュのワールド行列の更新
	{
		auto ptr = m_MeshCB[m_FrameIndex].GetPtr<CbMesh>();
		ptr->World = Matrix::Identity;
	}

	// 変換パラメータの更新
	{
		auto fovY = DirectX::XMConvertToRadians(37.5f);
		auto aspect = static_cast<float>(Constants::WindowWidth) / static_cast<float>(Constants::WindowHeight);

		auto ptr = m_TransformCB[m_FrameIndex].GetPtr<CbTransform>();
		ptr->View = Matrix::CreateLookAt(cameraPos, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
		ptr->Proj = Matrix::CreatePerspectiveFieldOfView(fovY, aspect, 1.0f, 1000.0f);
	}

	pCmdList->SetGraphicsRootSignature(m_SceneRootSignature.GetPtr());
	pCmdList->SetGraphicsRootDescriptorTable(0, m_TransformCB[m_FrameIndex].GetHandleGPU());
	pCmdList->SetGraphicsRootDescriptorTable(8, m_DirectionalLightCB[m_FrameIndex].GetHandleGPU());
	pCmdList->SetGraphicsRootDescriptorTable(2, m_LightCB[m_FrameIndex].GetHandleGPU());
	pCmdList->SetGraphicsRootDescriptorTable(3, m_CameraCB[m_FrameIndex].GetHandleGPU());
	pCmdList->SetPipelineState(m_pScenePSO.Get());
	pCmdList->RSSetViewports(1, &m_Viewport);
	pCmdList->RSSetScissorRects(1, &m_Scissor);

	// 描画
	{
		pCmdList->SetGraphicsRootDescriptorTable(1, m_MeshCB[m_FrameIndex].GetHandleGPU());
		DrawMesh(pCmdList);
	}
}

void D3D12Wrapper::DrawMesh(ID3D12GraphicsCommandList* pCmdList)
{
	for (size_t i = 0; i < m_pMeshes.size(); ++i)
	{
		// マテリアルIDを取得
		auto id = m_pMeshes[i]->GetMaterialId();

		// テクスチャを設定
		pCmdList->SetGraphicsRootDescriptorTable(4, m_Material.GetTextureHandle(id, TU_BASE_COLOR));
		pCmdList->SetGraphicsRootDescriptorTable(5, m_Material.GetTextureHandle(id, TU_METALLIC));
		pCmdList->SetGraphicsRootDescriptorTable(6, m_Material.GetTextureHandle(id, TU_ROUGHNESS));
		pCmdList->SetGraphicsRootDescriptorTable(7, m_Material.GetTextureHandle(id, TU_NORMAL));

		// メッシュを描画
		m_pMeshes[i]->Draw(pCmdList);
	}
}

void D3D12Wrapper::DrawTonemap(ID3D12GraphicsCommandList* pCmdList)
{
	// 定数バッファ更新
	{
		auto ptr = m_TonemapCB[m_FrameIndex].GetPtr<CbTonemap>();
		ptr->Type = m_TonemapType;
		ptr->ColorSpace = m_ColorSpace;
		ptr->BaseLuminance = m_BaseLuminance;
		ptr->MaxLuminance = m_MaxLuminance;
	}

	pCmdList->SetGraphicsRootSignature(m_TonemapRootSignature.GetPtr());
	pCmdList->SetGraphicsRootDescriptorTable(0, m_TonemapCB[m_FrameIndex].GetHandleGPU());
	pCmdList->SetGraphicsRootDescriptorTable(1, m_SceneColorTarget.GetHandleSRV()->HandleGPU);

	pCmdList->SetPipelineState(m_pTonemapPSO.Get());
	pCmdList->RSSetViewports(1, &m_Viewport);
	pCmdList->RSSetScissorRects(1, &m_Scissor);

	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	auto vbView = m_QuadVB.GetView();  // 一時変数に代入
	pCmdList->IASetVertexBuffers(0, 1, &vbView);
	pCmdList->DrawInstanced(3, 1, 0, 0);
}
