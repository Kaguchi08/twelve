#pragma once
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <map>
#include <memory>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <d3dx12.h>

class Dx12Wrapper;
class PMDRenderer;
class PMDActor;

enum GameState
{
	Title,
	Play,
	Pause,
};

class Game
{
public:
	Game();

	bool Initialize();
	void RunLoop();
	void Shutdown();

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
	HWND GetWindowHandle() const { return hwnd_; }
private:
	WNDCLASSEX wind_class_;
	HWND hwnd_;

	std::shared_ptr<Dx12Wrapper> dx12_;
	std::shared_ptr<class Renderer> renderer_;
	class InputSystem* input_system_ = nullptr;

	std::shared_ptr<PMDRenderer> mPMDRenderer;
	std::shared_ptr<PMDActor> mPMDActor;

	class Scene* current_scene_ = nullptr;
	class Scene* next_scene_ = nullptr;

	uint32_t tick_count_ = 0;

	GameState game_state_ = Play;

	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();
	void LoadData();
	void UnloadData();

	void CreateGameWindow(HWND& hwnd, WNDCLASSEX& wndClass);


};