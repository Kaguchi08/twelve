#include "Game.h"

#include "D3D12Wrapper.h"
#include "InputSystem.h"
#include "Constants.h"

namespace
{
	const auto ClassName = TEXT("DirectX");  //!< ウィンドウクラス名です.
}

Game::Game()
	: m_hInst(nullptr),
	m_hWnd(nullptr),
	m_width(Constants::WindowWidth),
	m_height(Constants::WindowHeight),
	m_pD3D12(nullptr),
	m_pInputSystem(nullptr),
	m_pScene(nullptr),
	m_state(State::Play)
{
}

Game::~Game()
{
}

bool Game::Initialize()
{
	// ウィンドウの初期化
	if (!InitWind())
	{
		return false;
	}

	// D3D12の初期化
	m_pD3D12 = std::make_shared<D3D12Wrapper>();
	if (!m_pD3D12->Initialize(m_hWnd))
	{
		return false;
	}

	// TODO: 適切な場所に移す
	// 描画処理の初期化
	/*if (!m_pD3D12->InitializeGraphicsPipeline())
	{
		return false;
	}*/

	if (!m_pD3D12->InitHDR())
	{
		return false;
	}

	// 初期化直後にHDRのサポートチェックを実行
	CheckSupportHDR();

	// 入力管理の初期化
	m_pInputSystem = std::make_shared<InputSystem>();
	if (!m_pInputSystem->Initialize(m_hWnd))
	{
		return false;
	}

	// シーンの初期化


	return true;
}

void Game::RunLoop()
{
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
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
	//m_pD3D12->ReleaseGraphicsResources();

	m_pD3D12->TermHDR();

	// D3D12の終了処理
	m_pD3D12->Terminate();

	// ウィンドウの終了処理
	TermWind();

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

bool Game::InitWind()
{
	// インスタンスハンドルを取得
	auto hInst = GetModuleHandle(nullptr);
	if (hInst == nullptr)
	{
		return false;
	}

	// ウィンドウの設定
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);
	wc.hCursor = LoadCursor(hInst, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = ClassName;
	wc.hIconSm = LoadIcon(hInst, IDI_APPLICATION);

	// ウィンドウの登録
	if (!RegisterClassEx(&wc))
	{
		return false;
	}

	// インスタンスハンドルの設定
	m_hInst = hInst;

	// ウィンドウサイズを設定
	RECT rc = {};
	rc.right = static_cast<LONG>(m_width);
	rc.bottom = static_cast<LONG>(m_height);

	// ウィンドウサイズを調整
	auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rc, style, FALSE);

	// ウィンドウを生成
	m_hWnd = CreateWindowEx(
		0,
		ClassName,
		TEXT("twelve"),
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		m_hInst,
		this);

	if (m_hWnd == nullptr)
	{
		return false;
	}

	// ウィンドウを表示
	ShowWindow(m_hWnd, SW_SHOWNORMAL);

	// ウィンドウを更新
	UpdateWindow(m_hWnd);

	// ウィンドウにフォーカスを設定
	SetFocus(m_hWnd);

	// 正常終了
	return true;
}

void Game::TermWind()
{
	if (m_hInst != nullptr)
	{
		UnregisterClass(ClassName, m_hInst);
	}

	m_hInst = nullptr;
	m_hWnd = nullptr;
}

void Game::CheckSupportHDR()
{
	if (m_pD3D12 == nullptr)
	{
		return;
	}

	m_pD3D12->CheckSupportHDR();
}

LRESULT Game::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	auto instance = reinterpret_cast<Game*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (msg)
	{
		case WM_CREATE:
		{
			auto pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lp);
			auto pGame = reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, pGame);
		} break;

		case WM_DESTROY:
		{
			PostQuitMessage(0);
		} break;

		case WM_MOVE:
		{
			instance->CheckSupportHDR();
		} break;

		case WM_DISPLAYCHANGE:
		{
			instance->CheckSupportHDR();
		} break;

		default:
		{
		} break;
	}

	return DefWindowProc(hWnd, msg, wp, lp);
}
