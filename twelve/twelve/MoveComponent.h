#pragma once
#include "Component.h"
#include <DirectXMath.h>
#include "XMFLOAT_Helper.h"

class MoveComponent : public Component
{
public:
	MoveComponent(class Actor* owner, int update_order = 100);

	void Update(float deltaTime) override;

	void SetVelocity(const DirectX::XMFLOAT3& velocity) { velocity_ = velocity; }
	void SetAngularVelocity(const DirectX::XMFLOAT3& angular_velocity) { angular_velocity_ = angular_velocity; }
	void SetAcceleration(const DirectX::XMFLOAT3& acceleration) { acceleration_ = acceleration; }
	void SetGravity(const DirectX::XMFLOAT3& gravity) { gravity_ = gravity; }
	void SetMaxSpeed(float max_speed) { max_speed_ = max_speed; }

	void AddVelocity(const DirectX::XMFLOAT3& velocity) { velocity_ += velocity; }
	void AddAngularVelocity(const DirectX::XMFLOAT3& angular_velocity) { angular_velocity_ += angular_velocity; }
	void AddAcceleration(const DirectX::XMFLOAT3& acceleration) { acceleration_ += acceleration; }

private:
	DirectX::XMFLOAT3 velocity_;
	DirectX::XMFLOAT3 angular_velocity_;
	DirectX::XMFLOAT3 acceleration_;
	DirectX::XMFLOAT3 gravity_;

	float max_speed_;
};
