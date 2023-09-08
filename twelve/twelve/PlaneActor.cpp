#include "PlaneActor.h"
#include "PrimitiveComponent.h"
#include "Scene.h"
#include "Game.h"
#include "Dx12Wrapper.h"

PlaneActor::PlaneActor(Scene* scene) :
	Actor(scene)
{
	primitive_ = new PrimitiveComponent(this, PrimitiveType::kPlane);
}

PlaneActor::~PlaneActor()
{
}

void PlaneActor::UpdateActor(float delta_time)
{
}

void PlaneActor::SetTexture(const char* file_name)
{
	primitive_->GetPrimitive()->CreateTextureAndView(scene_->GetGame()->GetDx12().get(), file_name);
}
