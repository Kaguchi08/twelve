#include "CameraComponent.h"
#include "Dx12Wrapper.h"
#include "Actor.h"
#include "Scene.h"
#include "Game.h"

CameraComponent::CameraComponent(Actor* owner, int update_order) :
	Component(owner, update_order)
{
}

void CameraComponent::SetViewMatrix(const DirectX::XMMATRIX& view)
{
	std::shared_ptr<Dx12Wrapper> dx12 = owner_->GetScene()->GetGame()->GetDx12();
	dx12->SetViewMatrix(view);
}
