#include "Game.h"

#include "DisplayManager.h"
#include "D3D12Wrapper.h"
#include "InputSystem.h"
#include "Constants.h"

Game::Game()
	: m_pD3D12(nullptr)
	, m_pInputSystem(nullptr)
	, m_pScene(nullptr)
	, m_state(State::Play)
{
}

Game::~Game()
{
}

bool Game::Initialize()
{
	// プラットフォーム層の初期化
	if (!m_Window.Initialize(Constants::WindowWidth, Constants::WindowHeight, "twelve"))
	{
		return false;
	}

	// 表示状態変更時のコールバックを設定
	m_Window.SetDisplayChangeCallback([this]()
	{
		// ここで HDR のチェックを行う
		Platform::DisplayManager displayManager;
		auto hdrInfo = displayManager.QueryHDRSupport(m_Window.GetHandle());
		m_pD3D12->SetHDRSupport(hdrInfo.supportsHDR);
		m_pD3D12->SetDisplayLuminance(hdrInfo.maxDisplayLuminance, hdrInfo.minDisplayLuminance);
	});

	// D3D12の初期化
	m_pD3D12 = std::make_shared<D3D12Wrapper>();
	if (!m_pD3D12->Initialize(m_Window.GetHandle()))
	{
		return false;
	}

	// 初回の HDR チェックを実行
	m_Window.TriggerDisplayChangeCallback();

	// TODO: 適切な場所に移す
	// 描画処理の初期化
	if (!m_pD3D12->InitializeGraphicsPipeline())
	{
		return false;
	}

	// 入力管理の初期化
	m_pInputSystem = std::make_shared<InputSystem>();
	if (!m_pInputSystem->Initialize(m_Window.GetHandle()))
	{
		return false;
	}

	// シーンの初期化


	return true;
}

void Game::RunLoop()
{
	MSG msg = {};
	while (true)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				return;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		ProcessInput();
		UpdateGame();
		GenerateOutput();
	}
}

void Game::Terminate()
{
	// 描画リソースの解放
	m_pD3D12->ReleaseGraphicsResources();

	// D3D12の終了処理
	m_pD3D12->Terminate();

	// リソースの解放
	if (m_pScene != nullptr)
	{
		m_pScene.reset();
	}

	if (m_pInputSystem != nullptr)
	{
		m_pInputSystem.reset();
	}

	if (m_pD3D12 != nullptr)
	{
		m_pD3D12.reset();
	}

	// プラットフォーム層の終了処理
	m_Window.Terminate();
}

void Game::ProcessInput()
{
	// 入力の更新
	m_pInputSystem->Update(m_state);

	auto& state = m_pInputSystem->GetState();
	if (state.keyboard.GetKeyState(VK_ESCAPE) == ButtonState::Pressed)
	{
		PostQuitMessage(0);
	}

	m_pD3D12->ProcessInput(state);
}

void Game::UpdateGame()
{
}

void Game::GenerateOutput()
{
	// 描画
	m_pD3D12->Render();
}
