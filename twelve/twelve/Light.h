#pragma once

#include <DirectXMath.h>

struct LightState
{
	// ディレクションライト
	DirectX::XMFLOAT3 direction;
	float padding;
	DirectX::XMFLOAT3 color;
	float padding2;
	// 環境光
	DirectX::XMFLOAT3 ambient;
	float padding3;
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
