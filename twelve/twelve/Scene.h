#pragma once
#include <vector>

class Scene {
public:
	Scene(class Game* game);
	
	void Update(float deltaTime);

	void AddActor(class Actor* actor);
	void RemoveActor(class Actor* actor);

	class Game* GetGame() const { return game_; }
private:
	virtual void UpdateActor(float deltaTime) = 0;

	class Game* game_;

	std::vector<class Actor*> actors_;
	std::vector<class Actor*> pending_actors_;
	bool is_update_actors_;
};