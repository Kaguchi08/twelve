#pragma once
#include "Actor.h"
#include <string>

class ModelActor : public Actor
{
public:
	ModelActor(class Scene* scene);
	~ModelActor();
	void UpdateActor(float delta_time) override;

	void SetPMDModel(const char* file_name);

private:
	class ModelComponent* model_;
	class ModelMoveComponent* move_;
};