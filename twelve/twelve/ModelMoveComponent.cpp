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

	if (state.keyboard.GetKeyDown('W'))
	{
		velocity.z += velocity_;
	}
	if (state.keyboard.GetKeyDown('S'))
	{
		velocity.z -= velocity_;
	}
	if (state.keyboard.GetKeyDown('A'))
	{
		velocity.x -= velocity_;
	}
	if (state.keyboard.GetKeyDown('D'))
	{
		velocity.x += velocity_;
	}

	SetInputVelocity(velocity);
}
