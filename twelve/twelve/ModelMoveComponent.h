#pragma once
#include <DirectXMath.h>

#include "MoveComponent.h"

class ModelMoveComponent : public MoveComponent {
   public:
    ModelMoveComponent(class Actor* owner, int update_order = 10);

    void ProcessInput(const struct InputState& state) override;

    float GetVelocity() const { return velocity_; }
    void SetVelocity(float velocity) { velocity_ = velocity; }

   private:
    float velocity_;

    DirectX::XMFLOAT3 HandleKeyboardInput(const InputState& state);
    void UpdateAnimation(bool isMoving);
};