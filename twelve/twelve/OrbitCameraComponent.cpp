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
	// ワールド上方ベクトル
	DirectX::XMFLOAT3 unit_y = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

	// ワールド上方を軸とするヨーのクォータニオンを作成
	DirectX::XMVECTOR yaw_quat = DirectX::XMQuaternionRotationAxis(DirectX::XMLoadFloat3(&unit_y), angular_velocity_.x);

	// カメラのオフセットと上方ベクトルを変換
	DirectX::XMStoreFloat3(&offset_, DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&offset_), yaw_quat));
	DirectX::XMStoreFloat3(&up_, DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&up_), yaw_quat));

	DirectX::XMFLOAT3 inv_offset = -offset_;

	// カメラの前方を計算
	DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&inv_offset));
	// カメラの右方を計算
	DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&up_), forward));

	// カメラ右方を軸とするピッチのクォータニオンを作成
	DirectX::XMVECTOR pitch_quat = DirectX::XMQuaternionRotationAxis(right, angular_velocity_.y);

	// カメラのオフセットと上方ベクトルを変換
	DirectX::XMStoreFloat3(&offset_, DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&offset_), pitch_quat));
	DirectX::XMStoreFloat3(&up_, DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&up_), pitch_quat));

	// 変換行列を計算
	DirectX::XMFLOAT3 target = owner_->GetPosition();
	DirectX::XMFLOAT3 camera_pos = target + offset_;
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&camera_pos), DirectX::XMLoadFloat3(&target), DirectX::XMLoadFloat3(&up_));

	// カメラの行列をセット
	SetViewMatrix(view);
}

void OrbitCameraComponent::ProcessInput(const InputState& state)
{
	// 角速度の更新
	DirectX::XMFLOAT2 angular_velocity_ = state.mouse.GetDelta() * sensitivity_;

	// 角速度の制限
	angular_velocity_.x = std::clamp(angular_velocity_.x, -max_angluar_velocity_, max_angluar_velocity_);
	angular_velocity_.y = std::clamp(angular_velocity_.y, -max_angluar_velocity_, max_angluar_velocity_);
}
