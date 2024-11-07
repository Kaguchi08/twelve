#include "Game.h"

#include <tchar.h>

#include "Dx12Wrapper.h"
#include "GameScene.h"
#include "InputSystem.h"
#include "Renderer.h"
#include "TestScene.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"

namespace {
const auto ClassName = TEXT("DirectX");  //!< ウィンドウクラス名です.
}

const unsigned int WINDOW_WIDTH = 1280;
const unsigned int WINDOW_HEIGHT = 720;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

Game::Game() : m_hInst(nullptr),
               m_hWnd(nullptr),
               m_Width(WINDOW_WIDTH),
               m_Height(WINDOW_HEIGHT) {
}

Game::~Game() {
}

bool Game::Initialize() {
    // ウィンドウの初期化
    if (!InitWnd()) {
        return false;
    }

    auto result = CoInitializeEx(0, COINIT_MULTITHREADED);

    dx12_.reset(new Dx12Wrapper(m_hWnd));

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
    // current_scene_ = new TestScene(this);

    tick_count_ = GetTickCount64();

    return true;
}

void Game::RunLoop() {
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

void Game::Terminate() {
    // ウィンドウの終了処理
    TermWnd();
}

SIZE Game::GetWindowSize() const {
    SIZE ret{};
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

bool Game::InitWnd() {
    // インスタンスハンドルを取得
    auto hInst = GetModuleHandle(nullptr);
    if (hInst == nullptr) {
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
    if (!RegisterClassEx(&wc)) {
        return false;
    }

    // インスタンスハンドルの設定
    m_hInst = hInst;

    // ウィンドウサイズを設定
    RECT rc = {};
    rc.right = static_cast<LONG>(m_Width);
    rc.bottom = static_cast<LONG>(m_Height);

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
        nullptr);

    if (m_hWnd == nullptr) {
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

void Game::TermWnd() {
    if (m_hInst != nullptr) {
        UnregisterClass(ClassName, m_hInst);
    }

    m_hInst = nullptr;
    m_hWnd = nullptr;
}

LRESULT CALLBACK Game::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;

        default: {
        } break;
    }

    ImGui_ImplWin32_WndProcHandler(hWnd, msg, wp, lp);

    return DefWindowProc(hWnd, msg, wp, lp);
}
