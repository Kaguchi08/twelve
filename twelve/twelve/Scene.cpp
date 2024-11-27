#include "Scene.h"

#include <algorithm>

#include "Actor.h"

Scene::Scene(std::shared_ptr<D3D12Wrapper> pD3D12)
	: m_pD3D12(pD3D12)
{
}

void Scene::Update(float deltaTime)
{
	for (auto& actor : m_pActors)
	{
		actor->Update(deltaTime);
	}
}

void Scene::AddActor(std::shared_ptr<Actor> pActor)
{
	m_pActors.push_back(pActor);
}

void Scene::RemoveActor(std::shared_ptr<Actor> pActor)
{
	auto iter = std::find(m_pActors.begin(), m_pActors.end(), pActor);
	if (iter != m_pActors.end())
	{
		std::iter_swap(iter, m_pActors.end() - 1);
		m_pActors.pop_back();
	}
}

void Scene::ActorInput(const InputState& state)
{
	for (auto& actor : m_pActors)
	{
		actor->ProcessInput(state);
	}
}
