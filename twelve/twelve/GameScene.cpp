#include "GameScene.h"
#include "ModelActor.h"
#include <string>

GameScene::GameScene(Game* game) :
	Scene(game)
{
	std::string model_path = "../Assets/Model/‰‰¹ƒ~ƒN.pmd";
	// ‰Šú‰»
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