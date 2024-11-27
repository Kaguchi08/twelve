#pragma once

#include <memory>
#include <vector>

class D3D12Wrapper;
class Actor;
struct InputState;

class Scene
{
public:
	Scene(std::shared_ptr<D3D12Wrapper> pD3D12);

	void Update(float deltaTime);

	void AddActor(std::shared_ptr<Actor> pActor);
	void RemoveActor(std::shared_ptr<Actor> pActor);

	void ActorInput(const InputState& state);

	virtual void ProcessInput(const InputState& state) = 0;

protected:
	std::shared_ptr<D3D12Wrapper> m_pD3D12;
	std::vector<std::shared_ptr<Actor>> m_pActors;

	virtual bool Initialize() = 0;
};
