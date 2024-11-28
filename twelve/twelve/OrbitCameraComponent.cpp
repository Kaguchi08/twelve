#include "OrbitCameraComponent.h"

#include <algorithm>

#include "Actor.h"
#include "InputSystem.h"
#include "XMFLOAT_Helper.h"

OrbitCameraComponent::OrbitCameraComponent(Actor* owner)
	: CameraComponent(owner),
	offset_(DirectX::XMFLOAT3(0.0f, 10.0f, -40.0f)),
	up_(DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)),
	angular_velocity_(DirectX::XMFLOAT2(0.0f, 0.0f)),
	max_angluar_velocity_(1.0f),
	sensitivity_(0.005f),
	max_angle_(DirectX::XM_PI / 6)
{
}

void OrbitCameraComponent::Update(float delta_time)
{
	// ワールド上方ベクトル
	DirectX::XMFLOAT3 unit_y = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

	// ワールド上方を軸とするヨーのクォータニオンを作成
	DirectX::XMVECTOR yaw_quat = DirectX::XMQuaternionRotationAxis(
		DirectX::XMLoadFloat3(&unit_y), angular_velocity_.x);

	// カメラのオフセットと上方ベクトルを変換
	DirectX::XMStoreFloat3(
		&offset_,
		DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&offset_), yaw_quat));
	DirectX::XMStoreFloat3(
		&up_, DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&up_), yaw_quat));

	DirectX::XMFLOAT3 inv_offset = -offset_;

	// カメラの前方を計算
	DirectX::XMVECTOR forward =
		DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&inv_offset));
	// カメラの右方を計算
	DirectX::XMVECTOR right = DirectX::XMVector3Normalize(
		DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&up_), forward));

	// カメラ右方を軸とするピッチのクォータニオンを作成
	DirectX::XMVECTOR pitch_quat =
		DirectX::XMQuaternionRotationAxis(right, angular_velocity_.y);

	// カメラのオフセットと上方ベクトルを変換
	DirectX::XMStoreFloat3(
		&offset_,
		DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&offset_), pitch_quat));
	DirectX::XMStoreFloat3(
		&up_, DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&up_), pitch_quat));

	// 変換行列を計算
	DirectX::XMFLOAT3 target = m_pOwner->GetPosition();
	// 注視点が低すぎるので少し上にずらす
	target.y += 10.0f;
	DirectX::XMFLOAT3 camera_pos = target + offset_;
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&camera_pos), DirectX::XMLoadFloat3(&target),
		DirectX::XMLoadFloat3(&up_));

	// カメラのxz平面における前方ベクトルを計算
	DirectX::XMFLOAT3 camera_forward_vector;
	camera_forward_vector.x = -offset_.x;
	camera_forward_vector.y = 0.0f;  // y成分は無視
	camera_forward_vector.z = -offset_.z;

	// カメラのxz平面における右ベクトルを計算
	DirectX::XMVECTOR camera_right_vector_xm =
		DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&unit_y),
								DirectX::XMLoadFloat3(&camera_forward_vector));

	// XMVECTORをXMFLOAT3に変換
	DirectX::XMFLOAT3 camera_right_vector;
	DirectX::XMStoreFloat3(&camera_right_vector, camera_right_vector_xm);

	// 正規化
	DirectX::XMStoreFloat3(&camera_right_vector,
						   ToNormalizeXMVECTOR(camera_right_vector));

	// Actorの前方ベクトルを更新
	m_pOwner->SetForward(DirectX::XMFLOAT3(camera_forward_vector));

	// Actorの右方ベクトルを更新
	m_pOwner->SetRight(DirectX::XMFLOAT3(camera_right_vector));

	// 視点座標のセット
	SetEyePosition(camera_pos);

	// カメラの行列をセット
	SetViewMatrix(view);

	// ターゲットの座標をセット
	SetTargetPosition(target);
}

void OrbitCameraComponent::ProcessInput(const InputState& state)
{
	// 角速度の更新
	angular_velocity_ = state.mouse.GetDelta() * sensitivity_;

	// 角速度の制限
	angular_velocity_.x = std::clamp(angular_velocity_.x, -max_angluar_velocity_,
									 max_angluar_velocity_);
	angular_velocity_.y = std::clamp(angular_velocity_.y, -max_angluar_velocity_,
									 max_angluar_velocity_);
}
