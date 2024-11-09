#pragma once

#include "D3D12Wrapper.h"
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <Windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

class Dx12Wrapper;

enum GameState
{
	kTitle,
	kPlay,
	kPause,
};

class Game
{
public:
	Game();
	~Game();

	bool Initialize();
	void RunLoop();
	void Terminate();

	// getter
	std::shared_ptr<Dx12Wrapper> GetDx12() const { return dx12_; }
	std::shared_ptr<class Renderer> GetRenderer() const { return renderer_; }
	class InputSystem* GetInputSystem() const { return input_system_; }
	uint32_t GetTickCount() const { return tick_count_; }
	GameState GetGameState() const { return game_state_; }

	// setter
	void SetGameState(GameState state) { game_state_ = state; }

	// Window
	SIZE GetWindowSize() const;
	HWND GetWindowHandle() const { return m_hWnd; }

private:
	// Window
	HINSTANCE m_hInst;
	HWND m_hWnd;
	uint32_t m_Width;
	uint32_t m_Height;

	D3D12Wrapper m_d3d12;
	std::shared_ptr<Dx12Wrapper> dx12_;
	std::shared_ptr<class Renderer> renderer_;
	class InputSystem* input_system_ = nullptr;

	class Scene* current_scene_ = nullptr;
	class Scene* next_scene_ = nullptr;

	uint32_t tick_count_ = 0;

	GameState game_state_ = kPlay;

	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();
	void LoadData();
	void UnloadData();

	bool InitWnd();
	void TermWnd();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
};
