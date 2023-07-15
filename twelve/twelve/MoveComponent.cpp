#include "MoveComponent.h"
#include "Actor.h"
#include "XMFLOAT_Helper.h"

MoveComponent::MoveComponent(Actor* owner, int update_order) :
	Component(owner, update_order)
{
	velocity_ = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	angular_velocity_ = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	acceleration_ = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	gravity_ = DirectX::XMFLOAT3(0.0f, -10.0f, 0.0f);
	max_speed_ = 100.0f;
}

void MoveComponent::Update(float deltaTime)
{
	// TODO 空中の移動に対応する

	// 速度を更新
	velocity_ += acceleration_ * deltaTime;

	// 速度を制限
	if (DirectX::XMVector3LengthSq(DirectX::XMLoadFloat3(&velocity_)).m128_f32[0] > max_speed_ * max_speed_)
	{
		// 正規化
		DirectX::XMStoreFloat3(&velocity_, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&velocity_)));
		velocity_ *= max_speed_;
	}

	// 位置を更新
	DirectX::XMFLOAT3 pos = owner_->GetPosition();
	pos += (velocity_ + input_velocity_) * deltaTime;

	// 回転を更新
	DirectX::XMFLOAT3 rot = owner_->GetRotation();
	rot += angular_velocity_ * deltaTime;
}