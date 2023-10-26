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
    // �E�B���h�E���j�����ꂽ��Ă΂��
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);  // OS�ɑ΂��ďI���Ɠ`����
        return 0;
    }

    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
    return DefWindowProc(hwnd, msg, wparam, lparam);  // �K��̏������s��
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

    // FPS�v�Z
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

    // �t���b�v
    dx12_->GetSwapChain()->Present(0, 0);
}

void Game::LoadData() {
}

void Game::UnloadData() {
}

void Game::CreateGameWindow(HWND& hwnd, WNDCLASSEX& wndClass) {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    // �E�B���h�E�N���X�������o�^
    wind_class_.cbSize = sizeof(WNDCLASSEX);
    wind_class_.lpfnWndProc = (WNDPROC)WindowProcedure;  // �R�[���o�b�N�֐��̎w��
    wind_class_.lpszClassName = _T("DirectXTest");       // �A�v���P�[�V�����N���X��(�K���ł����ł�)
    wind_class_.hInstance = GetModuleHandle(0);          // �n���h���̎擾
    RegisterClassEx(&wind_class_);                       // �A�v���P�[�V�����N���X(���������̍�邩���낵������OS�ɗ\������)

    RECT wrc = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};      // �E�B���h�E�T�C�Y�����߂�
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);  // �E�B���h�E�̃T�C�Y�͂�����Ɩʓ|�Ȃ̂Ŋ֐����g���ĕ␳����

    // �E�B���h�E�I�u�W�F�N�g�̐���
    hwnd = CreateWindow(
        wind_class_.lpszClassName,  // �N���X���w��
        _T("twelve"),               // �^�C�g���o�[�̕���
        WS_OVERLAPPEDWINDOW,        // �^�C�g���o�[�Ƌ��E��������E�B���h�E�ł�
        CW_USEDEFAULT,              // �\��X���W��OS�ɂ��C�����܂�
        CW_USEDEFAULT,              // �\��Y���W��OS�ɂ��C�����܂�
        wrc.right - wrc.left,       // �E�B���h�E��
        wrc.bottom - wrc.top,       // �E�B���h�E��
        nullptr,                    // �e�E�B���h�E�n���h��
        nullptr,                    // ���j���[�n���h��
        wind_class_.hInstance,      // �Ăяo���A�v���P�[�V�����n���h��
        nullptr);
}
