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
	input_velocity_ = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void MoveComponent::Update(float delta_time)
{
	// TODO �󒆂̈ړ��ɑΉ�����

	// ���x���X�V
	velocity_ += acceleration_ * delta_time;

	// ���x�𐧌�
	if (DirectX::XMVector3LengthSq(DirectX::XMLoadFloat3(&velocity_)).m128_f32[0] > max_speed_ * max_speed_)
	{
		// ���K��
		DirectX::XMStoreFloat3(&velocity_, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&velocity_)));
		velocity_ *= max_speed_;
	}

	// �ʒu���X�V
	DirectX::XMFLOAT3 pos = owner_->GetPosition();
	pos += (velocity_ + input_velocity_) * delta_time;
	owner_->SetPosition(pos);

	// ��]���X�V
	DirectX::XMFLOAT3 rot = owner_->GetRotation();
	rot += angular_velocity_ * delta_time;
	owner_->SetRotation(rot);

	input_velocity_ = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
}