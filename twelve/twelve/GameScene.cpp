#include "GameScene.h"
#include <string>

GameScene::GameScene(Game* game) :
	Scene(game)
{
	std::string model_path = "../Assets/Model/�����~�N.pmd";
	// ������
	Initialize(model_path.c_str());
}

GameScene::~GameScene()
{
}

bool GameScene::Initialize(const char* file_name)
{
	return false;
}

void GameScene::UpdateActor(float deltaTime)
{
}