#pragma once

class Scene {
public:
	Scene(class Game* game);
	
	void Update(float deltaTime);

	class Game* GetGame() { return game_; }
private:
	virtual void UpdateActor(float deltaTime) = 0;

	class Game* game_;

};