#pragma once
#include <DirectXMath.h>

#include "MoveComponent.h"

class ModelMoveComponent : public MoveComponent {
   public:
    ModelMoveComponent(class Actor* owner, int update_order = 10);

    void ProcessInput(const struct InputState& state) override;

   private:
    void UpdateAnimation(bool isMoving);
};