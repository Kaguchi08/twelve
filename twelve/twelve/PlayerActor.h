#pragma once
#include "Actor.h"
#include <string>

class PlayerActor : public Actor
{
public:
	PlayerActor(class Scene* scene);
	~PlayerActor();
	void UpdateActor(float delta_time) override;

	void SetPMDModel(const char* file_name);
	void SetFBXModel(const char* file_name);

private:
	class ModelComponent* model_;
	class ModelMoveComponent* move_;
};