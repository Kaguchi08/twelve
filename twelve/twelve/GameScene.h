#pragma once
#include "Scene.h"

class GameScene : public Scene
{
public:
	GameScene(class Game* game);
	~GameScene();
private:
	class ModelActor* model_;

	bool Initialize(const char* file_name);
	void UpdateActor(float deltaTime) override;
};