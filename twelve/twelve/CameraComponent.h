#pragma once
#include "Component.h"
#include <DirectXMath.h>

class CameraComponent : public Component
{
public:
	CameraComponent(class Actor* owner, int update_order = 100);

protected:
	void SetViewMatrix(const DirectX::XMMATRIX& view);
	void SetEyePosition(const DirectX::XMFLOAT3& eye);
	void SetTargetPosition(const DirectX::XMFLOAT3& target);
};