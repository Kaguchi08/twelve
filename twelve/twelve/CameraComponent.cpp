#include "CameraComponent.h"

#include "Actor.h"
#include "Dx12Wrapper.h"
#include "Game2.h"
#include "Scene.h"

CameraComponent::CameraComponent(Actor* owner, int update_order) : Component(owner, update_order)
{
}

void CameraComponent::SetViewMatrix(const DirectX::XMMATRIX& view)
{
	/*std::shared_ptr<Dx12Wrapper> dx12 = owner_->GetScene()->GetGame()->GetDx12();
	dx12->SetViewMatrix(view);*/
}

void CameraComponent::SetEyePosition(const DirectX::XMFLOAT3& eye)
{
	/*std::shared_ptr<Dx12Wrapper> dx12 = owner_->GetScene()->GetGame()->GetDx12();
	dx12->SetEye(eye);*/
}

void CameraComponent::SetTargetPosition(const DirectX::XMFLOAT3& target)
{
	/*std::shared_ptr<Dx12Wrapper> dx12 = owner_->GetScene()->GetGame()->GetDx12();
	dx12->SetTarget(target);*/
}
