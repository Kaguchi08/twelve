#include "GameScene.h"
#include "ModelActor.h"
#include <string>

GameScene::GameScene(Game* game) :
	Scene(game)
{
	std::string pmd_model_path = "../Assets/Model/‰‰¹ƒ~ƒN.pmd";

	// ‰Šú‰»
	Initialize(pmd_model_path.c_str());
}

GameScene::~GameScene()
{
}

bool GameScene::Initialize(const char* file_name)
{
	player_model_ = new PlayerActor(this);
	player_model_->SetPMDModel(file_name);

	std::string fbx_model_path = "../Assets/fbx/Box.fbx";
	//std::string fbx_model_path = "../Assets/fbx/building_04.fbx";
	//std::string fbx_model_path = "../Assets/fbx/Floor.FBX";

	return true;
}

void GameScene::UpdateActor(float deltaTime)
{
}