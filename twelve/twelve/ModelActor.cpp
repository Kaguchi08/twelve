#include "ModelActor.h"
#include "ModelComponent.h"

ModelActor::ModelActor(Game* game) :
	Actor(game),
	model_(nullptr)
{
}

ModelActor::~ModelActor()
{
}

void ModelActor::UpdateActor(float deltaTime)
{
}

void ModelActor::SetModel(const char* file_name)
{
	model_ = new ModelComponent(this, file_name);
}
