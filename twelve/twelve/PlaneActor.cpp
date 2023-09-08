#include "PlaneActor.h"
#include "PrimitiveComponent.h"

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
