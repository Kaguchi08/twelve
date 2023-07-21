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
	// �^�[�Q�b�g����̃I�t�Z�b�g
	DirectX::XMFLOAT3 offset_;
	// �J�����̏����
	DirectX::XMFLOAT3 up_;
	// ��]�̊p���x(yaw, pitch)
	DirectX::XMFLOAT2 angular_velocity_;
	// ��]�̊��x
	float sensitivity_;
	// ��]�̍ő�p���x
	float max_angluar_velocity_;
	// ��]�̍ő�p�x
	float max_angle_;

};