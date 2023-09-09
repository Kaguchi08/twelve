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

void PrimitiveComponent::Draw(bool is_shadow)
{
	auto cmd_list = dx12_->GetCommandList();

	switch (type_)
	{
		case kPlane:
			cmd_list->IASetVertexBuffers(0, 1, &primitive_->GetVertexBufferView());
			cmd_list->IASetIndexBuffer(&primitive_->GetIndexBufferView());
			cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			if (is_shadow)
			{
				cmd_list->DrawIndexedInstanced(primitive_->GetIndexNum(), 1, 0, 0, 0);
			}
			else
			{
				// テクスチャ
				cmd_list->SetDescriptorHeaps(1, primitive_->GetTextureSRVHeap().GetAddressOf());
				auto handle = primitive_->GetTextureSRVHeap()->GetGPUDescriptorHandleForHeapStart();
				cmd_list->SetGraphicsRootDescriptorTable(2, handle);

				// 法線マップ
				cmd_list->SetDescriptorHeaps(1, primitive_->GetNormalMapSRVHeap().GetAddressOf());
				handle = primitive_->GetNormalMapSRVHeap()->GetGPUDescriptorHandleForHeapStart();
				cmd_list->SetGraphicsRootDescriptorTable(4, handle);

				// ARMマップ
				cmd_list->SetDescriptorHeaps(1, primitive_->GetArmMapSRVHeap().GetAddressOf());
				handle = primitive_->GetArmMapSRVHeap()->GetGPUDescriptorHandleForHeapStart();
				cmd_list->SetGraphicsRootDescriptorTable(5, handle);

				cmd_list->DrawIndexedInstanced(primitive_->GetIndexNum(), 1, 0, 0, 0);
			}

			break;
		default:
			break;
	}
}
