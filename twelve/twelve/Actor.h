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

	Actor(class Scene* scene);
	virtual ~Actor();

	void Update(float delta_time);
	void UpdateComponents(float delta_time);
	virtual void UpdateActor(float delta_time);

	void ProcessInput(const uint8_t* keyState);
	virtual void ActorInput(const uint8_t* keyState);

	void AddComponent(class Component* component);
	void RemoveComponent(class Component* component);

	const DirectX::XMFLOAT3& GetPosition() const { return position_; }
	void SetPosition(const DirectX::XMFLOAT3& pos) { position_ = pos; }
	float GetScale() const { return scale_; }
	void SetScale(float scale) { scale_ = scale; }
	const DirectX::XMFLOAT3& GetRotation() const { return rotation_; }
	void SetRotation(const DirectX::XMFLOAT3& rotation) { rotation_ = rotation; }

	DirectX::XMFLOAT3 GetForward() const;
	DirectX::XMFLOAT3 GetRight() const;

	State GetState() const { return state_; }
	void SetState(State state) { state_ = state; }

	class Scene* GetScene() const { return scene_; }
private:
	State state_;
	class Scene* scene_;

	float scale_;
	DirectX::XMFLOAT3 position_;
	DirectX::XMFLOAT3 rotation_;

	std::vector<class Component*> components_;
};
