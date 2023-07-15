#pragma once
#include "Actor.h"
#include <string>

class ModelActor : public Actor
{
public:
	ModelActor(class Scene* scene);
	~ModelActor();
	void UpdateActor(float delta_time) override;

	void SetModel(const char* file_name);
private:
	class ModelComponent* model_;
};