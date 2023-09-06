#pragma once

#include <DirectXMath.h>

struct LightState
{
	// �f�B���N�V�������C�g
	DirectX::XMFLOAT3 direction;
	float padding;
	DirectX::XMFLOAT4 color;

	// ����
	DirectX::XMFLOAT4 ambient;
};

class Light
{
public:
	Light();
	~Light();

	// getter
	LightState* GetState() { return &state_; }

	// setter
	void SetState(const LightState& state) { state_ = state; }

private:
	LightState state_;

	void InitDirectionalLight(LightState& state);
	void InitAmbientLight(LightState& state);
};
