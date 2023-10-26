#include "ModelMoveComponent.h"

#include "Actor.h"
#include "InputSystem.h"
#include "PlayerActor.h"
#include "XMFLOAT_Helper.h"

ModelMoveComponent::ModelMoveComponent(Actor* owner, int update_order) : MoveComponent(owner, update_order),
                                                                         velocity_(50.0f) {
}

void ModelMoveComponent::ProcessInput(const InputState& state) {
    DirectX::XMFLOAT3 velocity = HandleKeyboardInput(state);

    if (DirectX::XMVector3Length(ToXMVECTOR(velocity)).m128_f32[0] > 0) {
        // 最終的な速度ベクトルを正規化
        DirectX::XMStoreFloat3(&velocity, ToNormalizeXMVECTOR(velocity));

        // 速度ベクトルから角度を計算（y軸周りの回転）
        float angle = atan2f(velocity.x, velocity.z);

        // 180度回転
        angle += DirectX::XM_PI;

        // ownerの向きを進む向きに設定
        owner_->SetRotation(DirectX::XMFLOAT3(0, angle, 0));

        velocity *= velocity_;

        SetInputVelocity(velocity);
    }

    UpdateAnimation(DirectX::XMVector3Length(ToXMVECTOR(velocity)).m128_f32[0] > 0);
}

DirectX::XMFLOAT3 ModelMoveComponent::HandleKeyboardInput(const InputState& state) {
    DirectX::XMFLOAT3 forward = owner_->GetForward();
    DirectX::XMFLOAT3 right = owner_->GetRight();

    DirectX::XMFLOAT3 forward_velocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    DirectX::XMFLOAT3 right_velocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

    // WASDキーの入力状態をチェック
    bool w_key = state.keyboard.GetKeyDown('W');
    bool a_key = state.keyboard.GetKeyDown('A');
    bool s_key = state.keyboard.GetKeyDown('S');
    bool d_key = state.keyboard.GetKeyDown('D');

    if (w_key) {
        forward_velocity += forward;
    }
    if (a_key) {
        right_velocity -= right;
    }
    if (s_key) {
        forward_velocity -= forward;
    }
    if (d_key) {
        right_velocity += right;
    }

    // 正規化
    DirectX::XMStoreFloat3(&forward_velocity, ToNormalizeXMVECTOR(forward_velocity));
    DirectX::XMStoreFloat3(&right_velocity, ToNormalizeXMVECTOR(right_velocity));

    return forward_velocity + right_velocity;
}

void ModelMoveComponent::UpdateAnimation(bool isMoving) {
    // アニメーションを更新
    PlayerActor* player = dynamic_cast<PlayerActor*>(owner_);
    if (player != nullptr) {
        player->SetCurrentAnimation(isMoving ? AnimationType::Run : AnimationType::Idle);
    }
}
