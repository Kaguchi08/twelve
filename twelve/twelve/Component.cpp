#include "Component.h"

#include "Actor.h"

Component::Component(Actor* owner)
	: m_pOwner(owner)
{
}

Component::~Component()
{
}

void Component::SetD3D12(std::shared_ptr<D3D12Wrapper> pD3D12)
{
	m_pD3D12 = pD3D12;
}
