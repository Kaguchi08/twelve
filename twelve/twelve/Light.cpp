#include "Light.h"

Light::Light() :
	state_()
{
	InitDirectionalLight(state_);
	InitAmbientLight(state_);
}

Light::~Light()
{
}

void Light::InitDirectionalLight(LightState& state)
{
	state.direction.x = 1.0f;
	state.direction.y = -1.0f;
	state.direction.z = 1.0f;

	state.color.x = 1.0f;
	state.color.y = 1.0f;
	state.color.z = 1.0f;
}

void Light::InitAmbientLight(LightState& state)
{
	state.ambient.x = 0.3f;
	state.ambient.y = 0.3f;
	state.ambient.z = 0.3f;
}