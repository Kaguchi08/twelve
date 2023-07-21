#include "OrbitCameraComponent.h"
#include "InputSystem.h"
#include "XMFLOAT_Helper.h"
#include "Actor.h"
#include <algorithm>


OrbitCameraComponent::OrbitCameraComponent(Actor* owner) :
	CameraComponent(owner),
	offset_(DirectX::XMFLOAT3(-40.f, 15.0f, 0.0f)),
	up_(DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)),
	max_angluar_velocity_(5.0f),
	sensitivity_(0.1f),
	max_angle_(DirectX::XM_PI)
{
}

void OrbitCameraComponent::Update(float delta_time)
{
	// ���[���h����x�N�g��
	DirectX::XMFLOAT3 unit_y = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

	// ���[���h��������Ƃ��郈�[�̃N�H�[�^�j�I�����쐬
	DirectX::XMVECTOR yaw_quat = DirectX::XMQuaternionRotationAxis(DirectX::XMLoadFloat3(&unit_y), angular_velocity_.x);

	// �J�����̃I�t�Z�b�g�Ə���x�N�g����ϊ�
	DirectX::XMStoreFloat3(&offset_, DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&offset_), yaw_quat));
	DirectX::XMStoreFloat3(&up_, DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&up_), yaw_quat));

	DirectX::XMFLOAT3 inv_offset = -offset_;

	// �J�����̑O�����v�Z
	DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&inv_offset));
	// �J�����̉E�����v�Z
	DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&up_), forward));

	// �J�����E�������Ƃ���s�b�`�̃N�H�[�^�j�I�����쐬
	DirectX::XMVECTOR pitch_quat = DirectX::XMQuaternionRotationAxis(right, angular_velocity_.y);

	// �J�����̃I�t�Z�b�g�Ə���x�N�g����ϊ�
	DirectX::XMStoreFloat3(&offset_, DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&offset_), pitch_quat));
	DirectX::XMStoreFloat3(&up_, DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&up_), pitch_quat));

	// �ϊ��s����v�Z
	DirectX::XMFLOAT3 target = owner_->GetPosition();
	DirectX::XMFLOAT3 camera_pos = target + offset_;
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&camera_pos), DirectX::XMLoadFloat3(&target), DirectX::XMLoadFloat3(&up_));

	// �J�����̍s����Z�b�g
	SetViewMatrix(view);
}

void OrbitCameraComponent::ProcessInput(const InputState& state)
{
	// �p���x�̍X�V
	DirectX::XMFLOAT2 angular_velocity_ = state.mouse.GetDelta() * sensitivity_;

	// �p���x�̐���
	angular_velocity_.x = std::clamp(angular_velocity_.x, -max_angluar_velocity_, max_angluar_velocity_);
	angular_velocity_.y = std::clamp(angular_velocity_.y, -max_angluar_velocity_, max_angluar_velocity_);
}
