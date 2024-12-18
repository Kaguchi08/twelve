﻿#pragma once
#include <string>

#include "Actor.h"
#include "ModelComponent.h"

class PlayerActor : public Actor
{
public:
	PlayerActor();
	~PlayerActor();
	void UpdateActor(float delta_time) override;

	void SetPMDModel(const char* file_name);
	void SetCurrentAnimation(AnimationType type) { current_animation_ = type; }

private:
	class ModelComponent* model_;
	class ModelMoveComponent* move_;

	AnimationType current_animation_;
};
