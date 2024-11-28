#include "AABBComponent.h"

#include "Actor.h"

AABBComponent::AABBComponent(Actor* owner) : Component(owner),
object_box_(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(0, 0, 0)),
world_box_(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(0, 0, 0)),
should_rotate_(true)
{
}

AABBComponent::~AABBComponent()
{
}

void AABBComponent::UpdateCollision()
{
	world_box_ = object_box_;

	// スケーリング
	world_box_.min.x *= m_pOwner->GetScale();
	world_box_.min.y *= m_pOwner->GetScale();

	// 回転
	if (should_rotate_)
	{
		world_box_.Rotate(m_pOwner->GetRotation());
	}

	// 移動
	world_box_.min += m_pOwner->GetPosition();
	world_box_.max += m_pOwner->GetPosition();
}
