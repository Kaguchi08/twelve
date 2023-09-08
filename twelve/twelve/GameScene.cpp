#include "GameScene.h"
#include "ModelActor.h"
#include "InputSystem.h"
#include "Game.h"
#include <string>

GameScene::GameScene(Game* game) :
	Scene(game),
	player_model_(nullptr),
	fbx_model_(nullptr),
	plane_actors_()
{
	std::string pmd_model_path = "../Assets/Model/初音ミクVer2.pmd";
	//std::string pmd_model_path = "../Assets/test/miku_gun.pmd";
	//std::string pmd_model_path = "../Assets/Model/巡音ルカ.pmd";
	//std::string pmd_model_path = "../Assets/Model/Kafka/kafka.pmd";

	// 初期化
	Initialize(pmd_model_path.c_str());
}

GameScene::~GameScene()
{
}

void GameScene::ProcessInput(const InputState& state)
{
	if (state.keyboard.GetKeyState(VK_RETURN) == ButtonState::kPressed)
	{
		if (game_->GetGameState() == GameState::kPause)
		{
			// カーソルを非表示
			ShowCursor(false);
			// 中央に戻す
			SetCursorPos(state.mouse.GetCenter().x, state.mouse.GetCenter().y);

			game_->SetGameState(GameState::kPlay);
		}
		else if (game_->GetGameState() == GameState::kPlay)
		{
			// カーソルを表示
			ShowCursor(true);
			game_->SetGameState(GameState::kPause);
		}
	}
}

bool GameScene::Initialize(const char* file_name)
{
	player_model_ = new PlayerActor(this);
	player_model_->SetPMDModel(file_name);

	std::string fbx_model_path = "../Assets/fbx/Plane.fbx";
	//std::string fbx_model_path = "../Assets/fbx/forest_nature_set_all_in.fbx";
	//std::string fbx_model_path = "../Assets/fbx/Floor.FBX";
	//std::string fbx_model_path = "../Assets/fbx/France_GameMap.fbx";
	fbx_model_ = new FBXActor(this);
	fbx_model_->SetFBXModel(fbx_model_path.c_str());

	plane_actors_.emplace_back(new PlaneActor(this));

	for (auto& plane_actotr : plane_actors_)
	{
		std::string tex_file_path = "../Assets/Textures/mossy_cobblestone_4k/textures/mossy_cobblestone_diff_4k.png";
		plane_actotr->SetTexture(tex_file_path.c_str());

		std::string normal_map_file_path = "../Assets/Textures/mossy_cobblestone_4k/textures/mossy_cobblestone_nor_dx_4k.png";
		plane_actotr->SetNormalMap(normal_map_file_path.c_str());
	}

	return true;
}

void GameScene::UpdateActor(float deltaTime)
{
}