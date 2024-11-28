#include "ModelMoveComponent.h"

#include "Actor.h"
#include "InputSystem.h"
#include "PlayerActor.h"
#include "XMFLOAT_Helper.h"

ModelMoveComponent::ModelMoveComponent(Actor* owner) : MoveComponent(owner)
{
	SetInputSpeed(50.0f);
}

void ModelMoveComponent::ProcessInput(const InputState& state)
{
	MoveComponent::ProcessInput(state);

	UpdateAnimation(DirectX::XMVector3Length(ToXMVECTOR(GetInputVelocity())).m128_f32[0] > 0);
}

void ModelMoveComponent::UpdateAnimation(bool isMoving)
{
	// アニメーションを更新
	PlayerActor* player = dynamic_cast<PlayerActor*>(m_pOwner);
	if (player != nullptr)
	{
		player->SetCurrentAnimation(isMoving ? AnimationType::Run : AnimationType::Idle);
	}
}
