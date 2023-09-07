#pragma once
#include <vector>

class Scene
{
public:
	Scene(class Game* game);

	void Update(float delta_time);

	void AddActor(class Actor* actor);
	void RemoveActor(class Actor* actor);

	void ActorInput(const struct InputState& state);

	virtual void ProcessInput(const struct InputState& state) = 0;

	class Game* GetGame() const { return game_; }
protected:
	virtual void UpdateActor(float delta_time) = 0;

	class Game* game_;

	std::vector<class Actor*> actors_;
	std::vector<class Actor*> pending_actors_;
	bool is_update_actors_;
};