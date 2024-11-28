#pragma once
#include <DirectXMath.h>

#include "Component.h"

class CameraComponent : public Component
{
public:
	CameraComponent(class Actor* owner);

protected:
	void SetViewMatrix(const DirectX::XMMATRIX& view);
	void SetEyePosition(const DirectX::XMFLOAT3& eye);
	void SetTargetPosition(const DirectX::XMFLOAT3& target);
};
