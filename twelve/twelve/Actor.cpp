#include "Actor.h"
#include "Scene.h"

Actor::Actor(class Scene* scene) :
	state_(State::EActive),
	scale_(1.0f),
	position_(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)),
	rotation_(DirectX::XMQuaternionIdentity()),
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

void Actor::Update(float deltaTime)
{
	if (state_ == State::EActive)
	{
		UpdateComponents(deltaTime);
		UpdateActor(deltaTime);
	}
}

void Actor::UpdateComponents(float deltaTime)
{
}

void Actor::UpdateActor(float deltaTime)
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
}

void Actor::RemoveComponent(Component* component)
{
}

DirectX::XMFLOAT3 Actor::GetForward() const
{
	return DirectX::XMFLOAT3();
}

DirectX::XMFLOAT3 Actor::GetRight() const
{
	return DirectX::XMFLOAT3();
}
