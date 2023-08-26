#include "AABBComponent.h"
#include "Actor.h"


AABBComponent::AABBComponent(Actor* owner, int update_order) :
	Component(owner, update_order),
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
	world_box_.min.x *= owner_->GetScale();
	world_box_.min.y *= owner_->GetScale();

	// 回転
	if (should_rotate_)
	{
		world_box_.Rotate(owner_->GetRotation());
	}

	// 移動
	world_box_.min += owner_->GetPosition();
	world_box_.max += owner_->GetPosition();
}
