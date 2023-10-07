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

	if (state.keyboard.GetKeyDown('W'))
	{
		forwardVelocity.x += forward.x;
		forwardVelocity.z += forward.z;
	}
	if (state.keyboard.GetKeyDown('S'))
	{
		forwardVelocity.x -= forward.x;
		forwardVelocity.z -= forward.z;
	}
	if (state.keyboard.GetKeyDown('A'))
	{
		rightVelocity.x -= right.x;
		rightVelocity.z -= right.z;
	}
	if (state.keyboard.GetKeyDown('D'))
	{
		rightVelocity.x += right.x;
		rightVelocity.z += right.z;
	}

	// 正規化
	DirectX::XMStoreFloat3(&forwardVelocity, ToNormalizeXMVECTOR(forwardVelocity));
	DirectX::XMStoreFloat3(&rightVelocity, ToNormalizeXMVECTOR(rightVelocity));

	velocity = forwardVelocity + rightVelocity;

	// 最終的な速度ベクトルを正規化
	DirectX::XMStoreFloat3(&velocity, ToNormalizeXMVECTOR(velocity));

	velocity *= velocity_;

	SetInputVelocity(velocity);
}
