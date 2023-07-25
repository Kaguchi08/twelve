#include "FBXActor.h"
#include "ModelComponent.h"
#include "Model.h"

FBXActor::FBXActor(Scene* scene) :
	Actor(scene),
	model_(nullptr)
{
}

FBXActor::~FBXActor()
{
}

void FBXActor::UpdateActor(float delta_time)
{
}

void FBXActor::SetFBXModel(const char* file_name)
{
	model_ = new ModelComponent(this, ModelType::FBX, file_name);
}
