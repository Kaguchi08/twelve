#pragma once
#include "Actor.h"

class PlaneActor : public Actor
{
public:
	PlaneActor();
	~PlaneActor();

	void UpdateActor(float delta_time) override;
	void SetTexture(const char* file_name);
	void SetNormalMap(const char* file_name);
	void SetArmMap(const char* file_name);

private:
	class PrimitiveComponent* primitive_;
};
