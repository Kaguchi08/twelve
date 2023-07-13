#include "GameScene.h"
#include "ModelActor.h"
#include <string>

GameScene::GameScene(Game* game) :
	Scene(game)
{
	std::string model_path = "../Assets/Model/初音ミク.pmd";
	// 初期化
	Initialize(model_path.c_str());
}

GameScene::~GameScene()
{
}

bool GameScene::Initialize(const char* file_name)
{
	model_ = new ModelActor(this);
	model_->SetModel(file_name);

	return true;
}

void GameScene::UpdateActor(float deltaTime)
{
}