#pragma once
#include "CameraComponent.h"

class OrbitCameraComponent : public CameraComponent
{
public:
	OrbitCameraComponent(class Actor* owner);

	void Update(float delta_time) override;
	void ProcessInput(const struct InputState& state) override;

	// getter
	const DirectX::XMFLOAT3& GetOffset() const { return offset_; }
	const DirectX::XMFLOAT3& GetUp() const { return up_; }
	const DirectX::XMFLOAT2& GetAngularVelocity() const { return angular_velocity_; }

	float GetMaxRotVelocity() const { return max_angluar_velocity_; }
	float GetSensitivity() const { return sensitivity_; }
	float GetMaxRotAngle() const { return max_angle_; }

	// setter
	void SetOffset(const DirectX::XMFLOAT3& offset) { offset_ = offset; }
	void SetUp(const DirectX::XMFLOAT3& up) { up_ = up; }
	void SetAngularVelocity(const DirectX::XMFLOAT2& angular_velocity) { angular_velocity_ = angular_velocity; }

	void SetMaxRotVelocity(float max_rot_velocity) { max_angluar_velocity_ = max_rot_velocity; }
	void SetSensitivity(float sensitivity) { sensitivity_ = sensitivity; }
	void SetMaxRotAngle(float max_rot_angle) { max_angle_ = max_rot_angle; }

private:
	// ターゲットからのオフセット
	DirectX::XMFLOAT3 offset_;
	// カメラの上方向
	DirectX::XMFLOAT3 up_;
	// 回転の角速度(yaw, pitch)
	DirectX::XMFLOAT2 angular_velocity_;
	// 回転の感度
	float sensitivity_;
	// 回転の最大角速度
	float max_angluar_velocity_;
	// 回転の最大角度
	float max_angle_;

};