#pragma once
#include "Actor.h"

class PlaneActor : public Actor
{
public:
	PlaneActor(class Scene* scene);
	~PlaneActor();

	void UpdateActor(float delta_time) override;

private:
	class PrimitiveComponent* primitive_;
};