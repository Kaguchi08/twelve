#include "Light.h"

Light::Light() :
	state_(),
	directional_light_dir_(1.0f, -1.0f, 1.0f),
	directional_light_color_(0.5, 0.5, 0.5, 1.0f),
	ambient_(0.3f, 0.3f, 0.3f, 1.0f)
{
}

Light::~Light()
{
}

void Light::InitDirectionalLight(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT4& color, LightState state)
{
	state.direction.x = direction.x;
	state.direction.y = direction.y;
	state.direction.z = direction.z;
	state.direction.w = 0.0f; // ディレクションライトなのでwは0

	state.color.x = color.x;
	state.color.y = color.y;
	state.color.z = color.z;
	state.color.w = 0.0f; // 基本的に0
}

void Light::InitAmbientLight(const DirectX::XMFLOAT4& color, LightState state)
{
	state.ambient.x = color.x;
	state.ambient.y = color.y;
	state.ambient.z = color.z;
	state.ambient.w = 0.0f; // 基本的に0
}
