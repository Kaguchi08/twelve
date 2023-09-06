#pragma once

#include <DirectXMath.h>

struct LightState
{
	// ディレクションライト
	DirectX::XMFLOAT3 direction;
	float padding;
	DirectX::XMFLOAT4 color;

	// 環境光
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
