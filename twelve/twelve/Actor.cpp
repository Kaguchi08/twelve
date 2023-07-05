#include "Actor.h"
#include "Game.h"

Actor::Actor(Game* game) :
	state_(State::EActive),
	scale_(1.0f),
	position_(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)),
	rotation_(DirectX::XMQuaternionIdentity()),
	game_(game)
{
	game_->AddActor(this);
}

Actor::~Actor()
{
	game_->RemoveActor(this);

	while (!mComponents.empty())
	{
		delete mComponents.back();
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
