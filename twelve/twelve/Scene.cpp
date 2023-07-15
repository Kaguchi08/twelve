#include "Scene.h"
#include "Actor.h"

Scene::Scene(Game* game) :
	game_(game),
	is_update_actors_(false)
{
}

void Scene::Update(float delta_time)
{
	is_update_actors_ = true;
	for (auto actor : actors_)
	{
		actor->Update(delta_time);
	}
	is_update_actors_ = false;
	for (auto pending : pending_actors_)
	{
		//pending->ComputeWorldTransform();
		actors_.emplace_back(pending);
	}
	pending_actors_.clear();
	std::vector<Actor*> dead_actors;
	for (auto actor : actors_)
	{
		if (actor->GetState() == Actor::State::EDead)
		{
			dead_actors.emplace_back(actor);
		}
	}
	for (auto actor : dead_actors)
	{
		delete actor;
	}
}

void Scene::AddActor(Actor* actor)
{
	if (is_update_actors_)
	{
		pending_actors_.emplace_back(actor);
	}
	else
	{
		actors_.emplace_back(actor);
	}
}

void Scene::RemoveActor(Actor* actor)
{
	auto iter = std::find(actors_.begin(), actors_.end(), actor);
	if (iter != actors_.end())
	{
		std::iter_swap(iter, actors_.end() - 1);
		actors_.pop_back();
	}
	else
	{
		iter = std::find(pending_actors_.begin(), pending_actors_.end(), actor);
		if (iter != pending_actors_.end())
		{
			std::iter_swap(iter, pending_actors_.end() - 1);
			pending_actors_.pop_back();
		}
	}
}

void Scene::ProcessInput(const InputState& state)
{
	for (auto actor : actors_)
	{
		actor->ProcessInput(state);
	}
}
