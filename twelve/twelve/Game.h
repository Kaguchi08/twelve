#pragma once

#include <Windows.h>
#include <cstdint>
#include <memory>

class D3D12Wrapper;
class InputSystem;
class Scene;

enum State
{
	Play,
	Pause,
};

class Game
{
public:
	Game();
	~Game();

	bool Initialize();
	void RunLoop();
	void Terminate();

private:
	// Window
	HINSTANCE m_hInst;
	HWND m_hWnd;
	uint32_t m_width;
	uint32_t m_height;

	std::shared_ptr<D3D12Wrapper> m_pD3D12;
	std::shared_ptr<InputSystem> m_pInputSystem;
	std::shared_ptr<Scene> m_pScene;

	State m_state;

	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();

	bool InitWind();
	void TermWind();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
};
