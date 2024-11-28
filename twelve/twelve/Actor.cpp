#include "Actor.h"

#include "Component.h"
#include "Scene.h"

Actor::Actor()
	: scale_(1.0f)
	, position_(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f))
	, rotation_(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f))
	, forward_(DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f))
	, right_(DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f))
{
}

Actor::~Actor()
{
	while (!m_pComponents.empty())
	{
		m_pComponents.back().reset();
	}
}

void Actor::Update(float deltaTime)
{
	for (auto& component : m_pComponents)
	{
		component->Update(deltaTime);
	}

	UpdateActor(deltaTime);
}

void Actor::ProcessInput(const InputState& state)
{
	for (auto& component : m_pComponents)
	{
		component->ProcessInput(state);
	}

	ActorInput(state);
}

void Actor::GenerateOutput()
{
	for (auto& component : m_pComponents)
	{
		component->GenerateOutput();
	}
}

void Actor::SetD3D12(std::shared_ptr<D3D12Wrapper> pD3D12)
{
	for (auto& component : m_pComponents)
	{
		component->SetD3D12(pD3D12);
	}
}

void Actor::AddComponent(std::shared_ptr<Component> pComponent)
{
	m_pComponents.push_back(pComponent);
}

void Actor::RemoveComponent(std::shared_ptr<Component> pComponent)
{
	auto iter = std::find(m_pComponents.begin(), m_pComponents.end(), pComponent);
	if (iter != m_pComponents.end())
	{
		m_pComponents.erase(iter);
	}
}
