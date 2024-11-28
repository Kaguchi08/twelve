#pragma once

#include <memory>

class Actor;
class D3D12Wrapper;
struct InputState;

class Component
{
public:
	Component(Actor* pOwner);
	~Component();

	void SetD3D12(std::shared_ptr<D3D12Wrapper> pD3D12);

	virtual void Update(float deltaTime) = 0;
	virtual void ProcessInput(const InputState& state) = 0;
	virtual void GenerateOutput() = 0;

protected:
	Actor* m_pOwner;
	std::shared_ptr<D3D12Wrapper> m_pD3D12;
};
