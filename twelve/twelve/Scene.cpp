#include "Scene.h"

Scene::Scene(Game* game) :
	game_(game),
	is_update_actors_(false)
{
}

void Scene::Update(float deltaTime)
{
}

void Scene::AddActor(Actor* actor)
{
	if (is_update_actors_) {
		pending_actors_.emplace_back(actor);
	}
	else {
		actors_.emplace_back(actor);
	}
}

void Scene::RemoveActor(Actor* actor)
{
	auto iter = std::find(actors_.begin(), actors_.end(), actor);
	if (iter != actors_.end()) {
		std::iter_swap(iter, actors_.end() - 1);
		actors_.pop_back();
	}
	else {
		iter = std::find(pending_actors_.begin(), pending_actors_.end(), actor);
		if (iter != pending_actors_.end()) {
			std::iter_swap(iter, pending_actors_.end() - 1);
			pending_actors_.pop_back();
		}
	}
}
