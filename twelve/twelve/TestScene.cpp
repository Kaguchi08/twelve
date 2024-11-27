#include "TestScene.h"

TestScene::TestScene(std::shared_ptr<D3D12Wrapper> pD3D12) : Scene(pD3D12)
{
	Initialize();
}

TestScene::~TestScene()
{
}

void TestScene::ProcessInput(const InputState& state)
{
}

bool TestScene::Initialize()
{
	return true;
}
