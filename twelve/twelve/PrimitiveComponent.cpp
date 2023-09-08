#include "PrimitiveComponent.h"
#include "Actor.h"
#include "Scene.h"
#include "Game.h"
#include "Renderer.h"

PrimitiveComponent::PrimitiveComponent(Actor* owner, PrimitiveType type, int draw_order) :
	Component(owner),
	type_(type)
{
	dx12_ = owner_->GetScene()->GetGame()->GetDx12();
	renderer_ = owner_->GetScene()->GetGame()->GetRenderer();

	renderer_->AddPrimitiveComponent(this);

	switch (type)
	{
		case kPlane:
			primitive_ = std::make_shared<Plane>();

			primitive_->CreateVertexBuffer(dx12_->GetDevice().Get());
			primitive_->CreateIndexBuffer(dx12_->GetDevice().Get());

			break;
		default:
			break;
	}
}

PrimitiveComponent::~PrimitiveComponent()
{
	renderer_->RemovePrimitiveComponent(this);
}

void PrimitiveComponent::Draw()
{
	auto cmd_list = dx12_->GetCommandList();

	switch (type_)
	{
		case kPlane:
			cmd_list->IASetVertexBuffers(0, 1, &primitive_->GetVertexBufferView());
			cmd_list->IASetIndexBuffer(&primitive_->GetIndexBufferView());
			cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			cmd_list->DrawIndexedInstanced(primitive_->GetIndexNum(), 1, 0, 0, 0);

			break;
		default:
			break;
	}
}
