#include "PlayerActor.h"

#include "Model.h"
#include "ModelComponent.h"
#include "ModelMoveComponent.h"
#include "OrbitCameraComponent.h"

PlayerActor::PlayerActor() : Actor(),
current_animation_(AnimationType::Idle),
model_(nullptr)
{
	move_ = new ModelMoveComponent(this);
	new OrbitCameraComponent(this);
}
PlayerActor::~PlayerActor()
{
}

void PlayerActor::UpdateActor(float delta_time)
{
	if (model_ != nullptr)
	{
		// アニメーションの更新
		model_->SetCurrentAnimation(current_animation_);
	}
}

void PlayerActor::SetPMDModel(const char* file_name)
{
	model_ = new ModelComponent(this, file_name);

	// アニメーション追加
	model_->AddAnimation("../motion/idle.vmd", AnimationType::Idle);
	model_->AddAnimation("../motion/run5.vmd", AnimationType::Run);
}
