#include "Game.h"

#include <tchar.h>

#include "Dx12Wrapper.h"
#include "GameScene.h"
#include "InputSystem.h"
#include "Renderer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"

const unsigned int WINDOW_WIDTH = 1280;
const unsigned int WINDOW_HEIGHT = 720;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    // ウィンドウが破棄されたら呼ばれる
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);  // OSに対して終わりと伝える
        return 0;
    }

    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
    return DefWindowProc(hwnd, msg, wparam, lparam);  // 規定の処理を行う
}

Game::Game() : hwnd_(nullptr),
               wind_class_({}) {
}

bool Game::Initialize() {
    auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
    CreateGameWindow(hwnd_, wind_class_);

    dx12_.reset(new Dx12Wrapper(hwnd_));

    if (!dx12_->Initialize()) {
        return false;
    }

    renderer_.reset(new Renderer(dx12_.get()));
    renderer_->Initialize();

    input_system_ = new InputSystem(this);

    if (!input_system_->Initialize()) {
        return false;
    }

    current_scene_ = new GameScene(this);

    tick_count_ = GetTickCount64();

    return true;
}

void Game::RunLoop() {
    ShowWindow(hwnd_, SW_SHOW);
    MSG msg = {};

    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) {
            break;
        }

        ProcessInput();

        UpdateGame();

        GenerateOutput();
    }
}

void Game::Shutdown() {
    UnregisterClass(wind_class_.lpszClassName, wind_class_.hInstance);
}

SIZE Game::GetWindowSize() const {
    SIZE ret;
    ret.cx = WINDOW_WIDTH;
    ret.cy = WINDOW_HEIGHT;
    return ret;
}

void Game::ProcessInput() {
    input_system_->Update();

    const InputState& state = input_system_->GetState();

    if (state.keyboard.GetKeyState(VK_ESCAPE) == ButtonState::kPressed) {
        PostQuitMessage(0);
    }

    if (game_state_ == GameState::kPlay) {
        current_scene_->ActorInput(state);
    }

    current_scene_->ProcessInput(state);
}

void Game::UpdateGame() {
    auto end_time = tick_count_ + 16;
    while (GetTickCount64() < end_time) {
        Sleep(1);
    }

    float delta_time = (GetTickCount64() - tick_count_) / 1000.0f;

    // FPS計算
    dx12_->SetFPS(1 / delta_time);

    tick_count_ = GetTickCount64();

    if (game_state_ == GameState::kPlay) {
        current_scene_->Update(delta_time);
    }
}

void Game::GenerateOutput() {
    dx12_->SetCameraSetting();

    renderer_->Draw(game_state_);

    dx12_->ExecuteCommand();

    // フリップ
    dx12_->GetSwapChain()->Present(0, 0);
}

void Game::LoadData() {
}

void Game::UnloadData() {
}

void Game::CreateGameWindow(HWND& hwnd, WNDCLASSEX& wndClass) {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    // ウィンドウクラス生成＆登録
    wind_class_.cbSize = sizeof(WNDCLASSEX);
    wind_class_.lpfnWndProc = (WNDPROC)WindowProcedure;  // コールバック関数の指定
    wind_class_.lpszClassName = _T("DirectXTest");       // アプリケーションクラス名(適当でいいです)
    wind_class_.hInstance = GetModuleHandle(0);          // ハンドルの取得
    RegisterClassEx(&wind_class_);                       // アプリケーションクラス(こういうの作るからよろしくってOSに予告する)

    RECT wrc = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};      // ウィンドウサイズを決める
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);  // ウィンドウのサイズはちょっと面倒なので関数を使って補正する

    // ウィンドウオブジェクトの生成
    hwnd = CreateWindow(
        wind_class_.lpszClassName,  // クラス名指定
        _T("twelve"),               // タイトルバーの文字
        WS_OVERLAPPEDWINDOW,        // タイトルバーと境界線があるウィンドウです
        CW_USEDEFAULT,              // 表示X座標はOSにお任せします
        CW_USEDEFAULT,              // 表示Y座標はOSにお任せします
        wrc.right - wrc.left,       // ウィンドウ幅
        wrc.bottom - wrc.top,       // ウィンドウ高
        nullptr,                    // 親ウィンドウハンドル
        nullptr,                    // メニューハンドル
        wind_class_.hInstance,      // 呼び出しアプリケーションハンドル
        nullptr);
}
