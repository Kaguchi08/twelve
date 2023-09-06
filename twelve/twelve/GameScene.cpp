#include "GameScene.h"
#include "ModelActor.h"
#include <string>

GameScene::GameScene(Game* game) :
	Scene(game)
{
	std::string pmd_model_path = "../Assets/Model/�����~�NVer2.pmd";
	//std::string pmd_model_path = "../Assets/test/miku_gun.pmd";
	//std::string pmd_model_path = "../Assets/Model/�������J.pmd";
	//std::string pmd_model_path = "../Assets/Model/Kafka/kafka.pmd";

	// ������
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
	//std::string fbx_model_path = "../Assets/fbx/France_GameMap.fbx";
	fbx_model_ = new FBXActor(this);
	fbx_model_->SetFBXModel(fbx_model_path.c_str());

	return true;
}

void GameScene::UpdateActor(float deltaTime)
{
}