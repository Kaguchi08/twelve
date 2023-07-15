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
	// �E�B���h�E���j�����ꂽ��Ă΂�܂�
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0); // OS�ɑ΂��ďI���Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam); // �K��̏������s��
}

Game::Game() :
	hwnd_(nullptr),
	wind_class_({})
{
}

bool Game::Initialize()
{
	std::string strModelPath = "../Assets/Model/�����~�N.pmd";
	//std::string str_model_path = "../Assets/Model/�������J.pmd";
	//std::string str_model_path = "../Assets/Model/Kafka/kafka.pmd";
	//std::string str_model_path = "../Assets/Model/Star_Rail/Seele/seele.pmd";
	//std::string str_model_path = "../Assets/Model/Star_Rail/Ting Yun/��_.pmd";
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
	// 1����
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

	// �t���b�v
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

	// �E�B���h�E�N���X�������o�^
	wind_class_.cbSize = sizeof(WNDCLASSEX);
	wind_class_.lpfnWndProc = (WNDPROC)WindowProcedure; // �R�[���o�b�N�֐��̎w��
	wind_class_.lpszClassName = _T("DirectXTest"); // �A�v���P�[�V�����N���X��(�K���ł����ł�)
	wind_class_.hInstance = GetModuleHandle(0); // �n���h���̎擾
	RegisterClassEx(&wind_class_); // �A�v���P�[�V�����N���X(���������̍�邩���낵������OS�ɗ\������)

	RECT wrc = { 0,0, WINDOW_WIDTH, WINDOW_HEIGHT }; // �E�B���h�E�T�C�Y�����߂�
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false); // �E�B���h�E�̃T�C�Y�͂�����Ɩʓ|�Ȃ̂Ŋ֐����g���ĕ␳����

	// �E�B���h�E�I�u�W�F�N�g�̐���
	hwnd = CreateWindow(
		wind_class_.lpszClassName, // �N���X���w��
		_T("twelve"), // �^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW, // �^�C�g���o�[�Ƌ��E��������E�B���h�E�ł�
		CW_USEDEFAULT, // �\��X���W��OS�ɂ��C�����܂�
		CW_USEDEFAULT, // �\��Y���W��OS�ɂ��C�����܂�
		wrc.right - wrc.left, // �E�B���h�E��
		wrc.bottom - wrc.top, // �E�B���h�E��
		nullptr, // �e�E�B���h�E�n���h��
		nullptr, // ���j���[�n���h��
		wind_class_.hInstance, // �Ăяo���A�v���P�[�V�����n���h��
		nullptr
	);
}
