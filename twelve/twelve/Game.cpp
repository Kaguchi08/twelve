#include "Game.h"
#include "Dx12Wrapper.h"
#include "PMDRenderer.h"
#include "PMDActor.h"
#include "Renderer.h"
#include "GameScene.h"
#include <tchar.h>

const unsigned int WINDOW_WIDTH = 1280;
const unsigned int WINDOW_HEIGHT = 720;

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ウィンドウが破棄されたら呼ばれます
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0); // OSに対して終わりと伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam); // 規定の処理を行う
}

Game::Game() :
	hwnd_(nullptr),
	wind_class_({})
{
}

bool Game::Initialize()
{
	std::string strModelPath = "../Assets/Model/初音ミク.pmd";
	//std::string str_model_path = "../Assets/Model/巡音ルカ.pmd";
	//std::string str_model_path = "../Assets/Model/Kafka/kafka.pmd";
	//std::string str_model_path = "../Assets/Model/Star_Rail/Seele/seele.pmd";
	//std::string str_model_path = "../Assets/Model/Star_Rail/Ting Yun/停雲.pmd";
	//std::string str_model_path = "../Assets/Model/Star_Rail/Stelle/stelle.pmd";


	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
	CreateGameWindow(hwnd_, wind_class_);

	dx12_.reset(new Dx12Wrapper(hwnd_));
	mPMDRenderer.reset(new PMDRenderer(*dx12_));

	renderer_.reset(new Renderer(*dx12_));

	if (!dx12_->Initialize())
	{
		return false;
	}

	mPMDRenderer->Initialize();
	renderer_->Initialize();

	mPMDActor.reset(new PMDActor(strModelPath.c_str(), *mPMDRenderer));
	mPMDActor->LoadVMDFile("../motion/squat2.vmd");
	dx12_->ExecuteCommand();

	mPMDActor->PlayAnimation();

	current_scene_ = new GameScene(this);

	return true;
}

void Game::RunLoop()
{
	ShowWindow(hwnd_, SW_SHOW);
	float angle = 0.0f;
	MSG msg = {};

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
		{
			break;
		}

		ProcessInput();
		UpdateGame();
		GenerateOutput();
	}
}

void Game::Shutdown()
{
	UnregisterClass(wind_class_.lpszClassName, wind_class_.hInstance);
}

SIZE Game::GetWindowSize() const
{
	SIZE ret;
	ret.cx = WINDOW_WIDTH;
	ret.cy = WINDOW_HEIGHT;
	return ret;
}

void Game::ProcessInput()
{
}

void Game::UpdateGame()
{
	auto end_time = tick_count_ + 16;
	while (GetTickCount() < end_time)
	{
		Sleep(1);
	}

	float delta_time = (GetTickCount() - tick_count_) / 1000.0f;

	mPMDActor->Update();
	current_scene_->Update(delta_time);
}

void Game::GenerateOutput()
{
	// 1枚目
	dx12_->PreDrawToPera();
	//mPMDRenderer->BeforeDraw();
	renderer_->BeforeDraw();
	dx12_->DrawToPera1();
	renderer_->Draw();
	//mPMDActor->DrawToBackBuffer();

	dx12_->DrawToPera2();

	dx12_->Clear();
	dx12_->DrawToBackBuffer();

	dx12_->EndDraw();

	// フリップ
	dx12_->GetSwapChain()->Present(0, 0);
}

void Game::LoadData()
{
}

void Game::UnloadData()
{
}

void Game::CreateGameWindow(HWND& hwnd, WNDCLASSEX& wndClass)
{
	HINSTANCE hInst = GetModuleHandle(nullptr);

	// ウィンドウクラス生成＆登録
	wind_class_.cbSize = sizeof(WNDCLASSEX);
	wind_class_.lpfnWndProc = (WNDPROC)WindowProcedure; // コールバック関数の指定
	wind_class_.lpszClassName = _T("DirectXTest"); // アプリケーションクラス名(適当でいいです)
	wind_class_.hInstance = GetModuleHandle(0); // ハンドルの取得
	RegisterClassEx(&wind_class_); // アプリケーションクラス(こういうの作るからよろしくってOSに予告する)

	RECT wrc = { 0,0, WINDOW_WIDTH, WINDOW_HEIGHT }; // ウィンドウサイズを決める
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false); // ウィンドウのサイズはちょっと面倒なので関数を使って補正する

	// ウィンドウオブジェクトの生成
	hwnd = CreateWindow(
		wind_class_.lpszClassName, // クラス名指定
		_T("twelve"), // タイトルバーの文字
		WS_OVERLAPPEDWINDOW, // タイトルバーと境界線があるウィンドウです
		CW_USEDEFAULT, // 表示X座標はOSにお任せします
		CW_USEDEFAULT, // 表示Y座標はOSにお任せします
		wrc.right - wrc.left, // ウィンドウ幅
		wrc.bottom - wrc.top, // ウィンドウ高
		nullptr, // 親ウィンドウハンドル
		nullptr, // メニューハンドル
		wind_class_.hInstance, // 呼び出しアプリケーションハンドル
		nullptr
	);
}
