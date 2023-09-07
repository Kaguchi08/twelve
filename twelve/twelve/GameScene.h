#pragma once
#include "Scene.h"
#include "FBXActor.h"
#include "PlayerActor.h"

class GameScene : public Scene
{
public:
	GameScene(class Game* game);
	~GameScene();

	void ProcessInput(const struct InputState& state) override;

private:
	PlayerActor* player_model_;
	FBXActor* fbx_model_;

	bool Initialize(const char* file_name);
	void UpdateActor(float deltaTime) override;
};