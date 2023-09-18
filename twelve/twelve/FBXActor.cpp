#include "FBXActor.h"
#include "FBXComponent.h"
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
	model_ = new FBXComponent(this, file_name);
}
