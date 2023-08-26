#pragma once

#include <DirectXMath.h>

struct LightState
{
	// ディレクションライト
	DirectX::XMFLOAT4 direction;
	DirectX::XMFLOAT4 color;

	// 環境光
	DirectX::XMFLOAT4 ambient;
};

class Light
{
public:
	Light();
	~Light();

	void InitDirectionalLight(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT4& color, LightState state);
	void InitAmbientLight(const DirectX::XMFLOAT4& color, LightState state);

	// getter
	const LightState& GetState() const { return state_; }

	const DirectX::XMFLOAT3& GetDirectionalLightDir() const { return directional_light_dir_; }
	const DirectX::XMFLOAT4& GetDirectionalLightColor() const { return directional_light_color_; }
	const DirectX::XMFLOAT4& GetAmbient() const { return ambient_; }

	// setter
	void SetDirectionalLightDir(const DirectX::XMFLOAT3& dir) { directional_light_dir_ = dir; }
	void SetDirectionalLightColor(const DirectX::XMFLOAT4& color) { directional_light_color_ = color; }
	void SetAmbient(const DirectX::XMFLOAT4& ambient) { ambient_ = ambient; }

private:
	LightState state_;

	DirectX::XMFLOAT3 directional_light_dir_;
	DirectX::XMFLOAT4 directional_light_color_;
	DirectX::XMFLOAT4 ambient_;
};
