#include "PlayerActor.h"
#include "ModelComponent.h"
#include "ModelMoveComponent.h"
#include "OrbitCameraComponent.h"
#include "Model.h"

PlayerActor::PlayerActor(Scene* scene) :
	Actor(scene),
	model_(nullptr)
{
	new OrbitCameraComponent(this);
	move_ = new ModelMoveComponent(this);

}
PlayerActor::~PlayerActor()
{
}

void PlayerActor::UpdateActor(float delta_time)
{
}

void PlayerActor::SetPMDModel(const char* file_name)
{
	model_ = new ModelComponent(this, file_name);

	// アニメーション追加
	model_->AddAnimation("../motion/run5.vmd");
}