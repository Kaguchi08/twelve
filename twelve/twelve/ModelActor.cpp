#include "ModelActor.h"
#include "ModelComponent.h"
#include "ModelMoveComponent.h"
#include "OrbitCameraComponent.h"

ModelActor::ModelActor(Scene* scene) :
	Actor(scene),
	model_(nullptr)
{
	move_ = new ModelMoveComponent(this);
	new OrbitCameraComponent(this);
}

ModelActor::~ModelActor()
{
}

void ModelActor::UpdateActor(float delta_time)
{
}

void ModelActor::SetModel(const char* file_name)
{
	model_ = new ModelComponent(this, file_name);
}
