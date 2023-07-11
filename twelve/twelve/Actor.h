#pragma once
#include <vector>
#include <cstdint>
#include <DirectXMath.h>

class Actor
{
public:
	enum State
	{
		EActive,
		EPaused,
		EDead
	};

	Actor(class Game* game);
	virtual ~Actor();

	void Update(float deltaTime);
	void UpdateComponents(float deltaTime);
	virtual void UpdateActor(float deltaTime);

	void ProcessInput(const uint8_t* keyState);
	virtual void ActorInput(const uint8_t* keyState);

	void AddComponent(class Component* component);
	void RemoveComponent(class Component* component);

	const DirectX::XMFLOAT3& GetPosition() const { return position_; }
	void SetPosition(const DirectX::XMFLOAT3& pos) { position_ = pos; }
	float GetScale() const { return scale_; }
	void SetScale(float scale) { scale_ = scale; }
	const DirectX::XMVECTOR& GetRotation() const { return rotation_; }
	void SetRotation(const DirectX::XMVECTOR& rotation) { rotation_ = rotation; }

	DirectX::XMFLOAT3 GetForward() const;
	DirectX::XMFLOAT3 GetRight() const;

	State GetState() const { return state_; }
	void SetState(State state) { state_ = state; }

	class Game* GetGame() { return game_; }
private:
	State state_;
	class Game* game_;

	float scale_;
	DirectX::XMFLOAT3 position_;
	DirectX::XMVECTOR rotation_;

	std::vector<class Component*> components_;
};
