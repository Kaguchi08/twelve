#pragma once

#include <Windows.h>
#include <cstdint>
#include <memory>
#include "PlatformWindow.h"

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

	State GetState() const { return m_state; }

private:
	// Window
	Platform::Window m_Window;


	std::shared_ptr<D3D12Wrapper> m_pD3D12;
	std::shared_ptr<InputSystem> m_pInputSystem;
	std::shared_ptr<Scene> m_pScene;

	State m_state;

	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();
};
