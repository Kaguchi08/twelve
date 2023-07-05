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

class Game
{
public:
	Game();

	bool Initialize();
	void RunLoop();
	void Shutdown();

	void AddActor(class Actor* actor);
	void RemoveActor(class Actor* actor);

	// Window
	SIZE GetWindowSize() const;
private:
	WNDCLASSEX wind_class_;
	HWND hwnd_;

	std::shared_ptr<Dx12Wrapper> dx12_;
	std::shared_ptr<PMDRenderer> mPMDRenderer;
	std::shared_ptr<PMDActor> mPMDActor;

	std::vector<class Actor*> actors_;
	std::vector<class Actor*> pending_actors_;
	bool is_update_actors_;

	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();

	void CreateGameWindow(HWND& hwnd, WNDCLASSEX& wndClass);


};