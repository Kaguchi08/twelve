#pragma once

#include "Scene.h"

class TestScene : public Scene
{
public:
	TestScene(std::shared_ptr<D3D12Wrapper> pD3D12);
	~TestScene();

	void ProcessInput(const InputState& state) override;

private:
	bool Initialize();
};
