#include "TestScene.h"

#include <string>

#include "Game2.h"
#include "InputSystem.h"

TestScene::TestScene(Game2* game) : Scene(game)
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
	// FBXモデルの初期化
	std::string fbx_name = "horse_statue_01";
	std::string fbx_model_path = "../Assets/fbx/" + fbx_name + "_4k.fbx";
	std::string normal_map_path = "../Assets/fbx/Texture/" + fbx_name + "_nor_dx_4k.png";
	std::string arm_map_path = "../Assets/fbx/Texture/" + fbx_name + "_arm_4k.png";

	fbx_actor_table.push_back(
		CreateFBXActor(
			fbx_model_path.c_str(),
			normal_map_path.c_str(),
			arm_map_path.c_str(),
			50,
			DirectX::XMFLOAT3(-25, 10.5, -2)));

	fbx_name = "sofa_03";
	fbx_model_path = "../Assets/fbx/" + fbx_name + "_4k.fbx";
	normal_map_path = "../Assets/fbx/Texture/" + fbx_name + "_nor_dx_4k.png";
	arm_map_path = "../Assets/fbx/Texture/" + fbx_name + "_arm_4k.png";

	fbx_actor_table.push_back(
		CreateFBXActor(
			fbx_model_path.c_str(),
			normal_map_path.c_str(),
			arm_map_path.c_str(),
			15,
			DirectX::XMFLOAT3(20, 0, 30)));

	fbx_name = "CoffeeTable_01";
	fbx_model_path = "../Assets/fbx/" + fbx_name + "_4k.fbx";
	normal_map_path = "../Assets/fbx/Texture/" + fbx_name + "_nor_dx_4k.png";
	arm_map_path = "../Assets/fbx/Texture/" + fbx_name + "_arm_4k.png";

	fbx_actor_table.push_back(
		CreateFBXActor(
			fbx_model_path.c_str(),
			normal_map_path.c_str(),
			arm_map_path.c_str(),
			20,
			DirectX::XMFLOAT3(-20, 0, 0)));

	// Planeの初期化
	std::string name = "patterned_concrete_pavers_02";

	std::string tex_file_path = "../Assets/Textures/" + name + "_4k/textures/" + name + "_diff_4k.png";
	std::string normal_map_file_path = "../Assets/Textures/" + name + "_4k/textures/" + name + "_nor_dx_4k.png";
	std::string arm_map_file_path = "../Assets/Textures/" + name + "_4k/textures/" + name + "_arm_4k.png";

	plane_actor_table_.push_back(
		CreatePlaneActor(
			tex_file_path.c_str(),
			normal_map_file_path.c_str(),
			arm_map_file_path.c_str()));

	return true;
}

void TestScene::UpdateActor(float delta_time)
{
}

FBXActor* TestScene::CreateFBXActor(const char* model, const char* normal, const char* arm, const float scale, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot)
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

PlaneActor* TestScene::CreatePlaneActor(const char* tex, const char* normal, const char* arm, const float scale, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot)
{
	PlaneActor* plane_actor = new PlaneActor(this);

	plane_actor->SetTexture(tex);
	plane_actor->SetNormalMap(normal);
	plane_actor->SetArmMap(arm);
	plane_actor->SetScale(scale);
	plane_actor->SetPosition(pos);
	plane_actor->SetRotation(rot);

	return plane_actor;
}
