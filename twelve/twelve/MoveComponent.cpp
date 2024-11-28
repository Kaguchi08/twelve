#include "MoveComponent.h"

#include "Actor.h"
#include "InputSystem.h"
#include "XMFLOAT_Helper.h"

MoveComponent::MoveComponent(Actor* owner) : Component(owner)
{
	velocity_ = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	angular_velocity_ = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	acceleration_ = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	input_speed_ = 0.0f;
	max_speed_ = 100.0f;
	input_velocity_ = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void MoveComponent::Update(float delta_time)
{
	// 速度を更新
	velocity_ += acceleration_ * delta_time;

	// 速度を制限
	if (DirectX::XMVector3LengthSq(DirectX::XMLoadFloat3(&velocity_)).m128_f32[0] > max_speed_ * max_speed_)
	{
		// 正規化
		DirectX::XMStoreFloat3(&velocity_, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&velocity_)));
		velocity_ *= max_speed_;
	}

	// 位置を更新
	DirectX::XMFLOAT3 pos = m_pOwner->GetPosition();
	pos += (velocity_ + input_velocity_) * delta_time;
	m_pOwner->SetPosition(pos);

	// 回転を更新
	DirectX::XMFLOAT3 rot = m_pOwner->GetRotation();
	rot += angular_velocity_ * delta_time;
	m_pOwner->SetRotation(rot);

	input_velocity_ = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void MoveComponent::ProcessInput(const InputState& state)
{
	DirectX::XMFLOAT3 velocity = CalculateMovementVector(state);

	if (DirectX::XMVector3Length(ToXMVECTOR(velocity)).m128_f32[0] > 0)
	{
		// 最終的な速度ベクトルを正規化
		DirectX::XMStoreFloat3(&velocity, ToNormalizeXMVECTOR(velocity));

		// 水平成分（XZ平面）のみを使って角度を計算（y軸周りの回転）
		float angle = atan2f(velocity.x, velocity.z);

		// 180度回転
		angle += DirectX::XM_PI;

		// ownerの向きを進む向きに設定（Y軸方向の回転のみを対象）
		m_pOwner->SetRotation(DirectX::XMFLOAT3(0, angle, 0));

		// 速度ベクトルに対してスケール適用
		velocity *= input_speed_;

		SetInputVelocity(velocity);
	}
}

DirectX::XMFLOAT3 MoveComponent::CalculateMovementVector(const InputState& state)
{
	DirectX::XMFLOAT3 forward = m_pOwner->GetForward();
	DirectX::XMFLOAT3 right = m_pOwner->GetRight();
	DirectX::XMFLOAT3 up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

	DirectX::XMFLOAT3 forward_velocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 right_velocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 up_velocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	// WASDQEキーの入力状態をチェック
	bool w_key = state.keyboard.GetKeyDown('W');
	bool a_key = state.keyboard.GetKeyDown('A');
	bool s_key = state.keyboard.GetKeyDown('S');
	bool d_key = state.keyboard.GetKeyDown('D');
	bool q_key = state.keyboard.GetKeyDown('Q');
	bool e_key = state.keyboard.GetKeyDown('E');

	if (w_key)
	{
		forward_velocity += forward;
	}
	if (a_key)
	{
		right_velocity -= right;
	}
	if (s_key)
	{
		forward_velocity -= forward;
	}
	if (d_key)
	{
		right_velocity += right;
	}
	if (q_key)
	{
		up_velocity -= up;
	}
	if (e_key)
	{
		up_velocity += up;
	}

	// 正規化
	DirectX::XMStoreFloat3(&forward_velocity, ToNormalizeXMVECTOR(forward_velocity));
	DirectX::XMStoreFloat3(&right_velocity, ToNormalizeXMVECTOR(right_velocity));
	DirectX::XMStoreFloat3(&up_velocity, ToNormalizeXMVECTOR(up_velocity));

	return forward_velocity + right_velocity + up_velocity;
}
