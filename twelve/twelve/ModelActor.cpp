#include "ModelActor.h"
#include "ModelComponent.h"
#include "ModelMoveComponent.h"

ModelActor::ModelActor(Scene* scene) :
	Actor(scene),
	model_(nullptr)
{
	new ModelMoveComponent(this);
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
