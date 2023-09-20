#include "GameScene.h"
#include "InputSystem.h"
#include "Game.h"
#include <string>

GameScene::GameScene(Game* game) :
	Scene(game),
	player_model_(nullptr),
	plane_info_table_()
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

	//std::string fbx_model_path = "../Assets/fbx/Cube03.fbx";
	std::string fbx_model_path = "../Assets/fbx/horse_statue_01_4k.fbx";

	std::string normal_map_path = "../Assets/fbx/Texture/horse_statue_01_nor_dx_4k.png";
	std::string arm_map_path = "../Assets/fbx/Texture/horse_statue_01_arm_4k.png";

	fbx_actor_table.push_back
	(
		CreateFBXActor
		(
			fbx_model_path.c_str(),
			normal_map_path.c_str(),
			arm_map_path.c_str(),
			200
		)
	);

	PlaneInfo* plane_info = new PlaneInfo();

	plane_info->tex_file_path = "../Assets/Textures/mossy_cobblestone_4k/textures/mossy_cobblestone_diff_4k.png";
	plane_info->normal_map_file_path = "../Assets/Textures/mossy_cobblestone_4k/textures/mossy_cobblestone_nor_dx_4k.png";
	plane_info->arm_map_file_path = "../Assets/Textures/mossy_cobblestone_4k/textures/mossy_cobblestone_arm_4k.png";

	auto plane_actor = new PlaneActor(this);

	plane_info->plane_actor = plane_actor;

	plane_info_table_.push_back(plane_info);


	for (auto& plane_info : plane_info_table_)
	{
		std::string tex_file_path = plane_info->tex_file_path;
		plane_info->plane_actor->SetTexture(tex_file_path.c_str());

		std::string normal_map_file_path = plane_info->normal_map_file_path;
		plane_info->plane_actor->SetNormalMap(normal_map_file_path.c_str());

		std::string arm_map_file_path = plane_info->arm_map_file_path;
		plane_info->plane_actor->SetArmMap(arm_map_file_path.c_str());
	}

	return true;
}

void GameScene::UpdateActor(float deltaTime)
{
}

FBXActor* GameScene::CreateFBXActor(const char* model, const char* normal, const char* arm, const float scale, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot)
{
	FBXActor* fbx_actor = new FBXActor(this);

	fbx_actor->SetFBXModel(model);
	fbx_actor->SetNormalMap(normal);
	fbx_actor->SetArmMap(arm);
	fbx_actor->SetScale(scale);
	fbx_actor->SetPosition(pos);
	fbx_actor->SetRotation(rot);

	return fbx_actor;
}
