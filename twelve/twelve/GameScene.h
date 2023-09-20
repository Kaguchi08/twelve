#pragma once
#include "Scene.h"
#include "FBXActor.h"
#include "PlayerActor.h"
#include "PlaneActor.h"
#include <unordered_map>

struct PlaneInfo
{
	std::string tex_file_path;
	std::string normal_map_file_path;
	std::string arm_map_file_path;

	class PlaneActor* plane_actor;
};

class GameScene : public Scene
{
public:
	GameScene(class Game* game);
	~GameScene();

	void ProcessInput(const struct InputState& state) override;

private:
	PlayerActor* player_model_;

	std::vector<PlaneInfo*> plane_info_table_;
	std::vector<FBXActor*> fbx_actor_table;

	bool Initialize(const char* file_name);
	void UpdateActor(float deltaTime) override;

	FBXActor* CreateFBXActor(const char* model, const char* normal, const char* arm,
							 const float scale = 1.0f,
							 const DirectX::XMFLOAT3& pos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
							 const DirectX::XMFLOAT3& rot = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
};