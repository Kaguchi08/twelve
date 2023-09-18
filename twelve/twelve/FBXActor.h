#pragma once
#include "Actor.h"
#include <string>

class FBXActor : public Actor
{
public:
	FBXActor(class Scene* scene);
	~FBXActor();
	void UpdateActor(float delta_time) override;

	void SetFBXModel(const char* file_name);

private:
	class FBXComponent* model_;
};