#include "ModelActor.h"
#include "ModelComponent.h"

ModelActor::ModelActor(Scene* scene) :
	Actor(scene),
	model_(nullptr)
{
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
