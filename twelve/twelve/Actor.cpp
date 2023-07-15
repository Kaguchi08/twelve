#include "Actor.h"
#include "Scene.h"
#include "Component.h"

Actor::Actor(class Scene* scene) :
	state_(State::EActive),
	scale_(1.0f),
	position_(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)),
	rotation_(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)),
	scene_(scene)
{
	scene_->AddActor(this);
}

Actor::~Actor()
{
	scene_->RemoveActor(this);

	while (!components_.empty())
	{
		delete components_.back();
	}
}

void Actor::Update(float delta_time)
{
	if (state_ == State::EActive)
	{
		UpdateComponents(delta_time);
		UpdateActor(delta_time);
	}
}

void Actor::UpdateComponents(float delta_time)
{
	for (auto component : components_)
	{
		component->Update(delta_time);
	}
}

void Actor::UpdateActor(float delta_time)
{
}

void Actor::ProcessInput(const uint8_t* keyState)
{
}

void Actor::ActorInput(const uint8_t* keyState)
{
}

void Actor::AddComponent(Component* component)
{
	components_.emplace_back(component);
}

void Actor::RemoveComponent(Component* component)
{
	auto iter = std::find(components_.begin(), components_.end(), component);
	if (iter != components_.end())
	{
		components_.erase(iter);
	}
}

DirectX::XMFLOAT3 Actor::GetForward() const
{
	return DirectX::XMFLOAT3();
}

DirectX::XMFLOAT3 Actor::GetRight() const
{
	return DirectX::XMFLOAT3();
}
