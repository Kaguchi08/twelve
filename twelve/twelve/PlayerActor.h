#pragma once
#include "Actor.h"
#include <string>
#include "ModelComponent.h"

class PlayerActor : public Actor
{
public:
	PlayerActor(class Scene* scene);
	~PlayerActor();
	void UpdateActor(float delta_time) override;

	void SetPMDModel(const char* file_name);
	void SetCurrentAnimation(AnimationType type) { current_animation_ = type; }

private:
	class ModelComponent* model_;
	class ModelMoveComponent* move_;

	AnimationType current_animation_;
};