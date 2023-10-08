#include "ModelMoveComponent.h"
#include "InputSystem.h"
#include "Actor.h"
#include "XMFLOAT_Helper.h"
#include <DirectXMath.h>

ModelMoveComponent::ModelMoveComponent(Actor* owner, int update_order) :
	MoveComponent(owner, update_order),
	velocity_(50.0f)
{
}

void ModelMoveComponent::ProcessInput(const InputState& state)
{
	DirectX::XMFLOAT3 velocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 forward = owner_->GetForward();
	DirectX::XMFLOAT3 right = owner_->GetRight();

	DirectX::XMFLOAT3 forwardVelocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 rightVelocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	// WASD�L�[�̓��͏�Ԃ��`�F�b�N
	bool w_key = state.keyboard.GetKeyDown('W');
	bool a_key = state.keyboard.GetKeyDown('A');
	bool s_key = state.keyboard.GetKeyDown('S');
	bool d_key = state.keyboard.GetKeyDown('D');

	if (w_key)
	{
		forwardVelocity += forward;
	}
	if (a_key)
	{
		rightVelocity -= right;
	}
	if (s_key)
	{
		forwardVelocity -= forward;
	}
	if (d_key)
	{
		rightVelocity += right;
	}

	// ���K��
	DirectX::XMStoreFloat3(&forwardVelocity, ToNormalizeXMVECTOR(forwardVelocity));
	DirectX::XMStoreFloat3(&rightVelocity, ToNormalizeXMVECTOR(rightVelocity));

	velocity = forwardVelocity + rightVelocity;

	if (w_key || a_key || s_key || d_key)
	{
		// �ŏI�I�ȑ��x�x�N�g���𐳋K��
		DirectX::XMStoreFloat3(&velocity, ToNormalizeXMVECTOR(velocity));

		// ���x�x�N�g������p�x���v�Z�iy������̉�]�j
		float angle = atan2f(velocity.x, velocity.z);

		// 180�x��]
		angle += DirectX::XM_PI;

		// owner�̌�����i�ތ����ɐݒ�
		owner_->SetRotation(DirectX::XMFLOAT3(0, angle, 0));

		velocity *= velocity_;

		SetInputVelocity(velocity);
	}
}
