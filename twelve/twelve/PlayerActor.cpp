#include "PlayerActor.h"
#include "ModelComponent.h"
#include "ModelMoveComponent.h"
#include "OrbitCameraComponent.h"
#include "Model.h"

PlayerActor::PlayerActor(Scene* scene) :
	Actor(scene),
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
}

void PlayerActor::SetPMDModel(const char* file_name)
{
	model_ = new ModelComponent(this, ModelType::PMD, file_name);

	// �A�j���[�V�����ǉ�
	model_->AddAnimation("../motion/run.vmd");
}

void PlayerActor::SetFBXModel(const char* file_name)
{
}
