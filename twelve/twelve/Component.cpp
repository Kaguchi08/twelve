#include "Component.h"
#include "Actor.h"

Component::Component(Actor* owner, int update_order) :
	owner_(owner),
	update_order_(update_order)
{
	owner_->AddComponent(this);
}

Component::~Component()
{
	owner_->RemoveComponent(this);
}

void Component::Update(float delta_time)
{
}
