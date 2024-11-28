#pragma once
#include <DirectXMath.h>

#include "MoveComponent.h"

class ModelMoveComponent : public MoveComponent
{
public:
	ModelMoveComponent(class Actor* owner);

	void ProcessInput(const struct InputState& state) override;
	void GenerateOutput() override {}

private:
	void UpdateAnimation(bool isMoving);
};
