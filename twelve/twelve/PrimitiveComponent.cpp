#include "PrimitiveComponent.h"

#include "Actor.h"
#include "Game2.h"
#include "Renderer.h"
#include "Scene.h"

PrimitiveComponent::PrimitiveComponent(Actor* owner, PrimitiveType type, int draw_order) : Component(owner),
type_(type)
{
	/*dx12_ = owner_->GetScene()->GetGame()->GetDx12();
	renderer_ = owner_->GetScene()->GetGame()->GetRenderer();*/

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

	// 座標変換用リソース作成
	auto result = CreateTransformResourceAndView();

	if (FAILED(result))
	{
		assert(0);
	}
}

PrimitiveComponent::~PrimitiveComponent()
{
	renderer_->RemovePrimitiveComponent(this);
}

void PrimitiveComponent::Update(float delta_time)
{
	// ワールド行列の更新
	auto pos = owner_->GetPosition();
	auto rot = owner_->GetRotation();

	*world_matrix_ = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
}

void PrimitiveComponent::Draw(bool is_shadow)
{
	auto cmd_list = dx12_->GetCommandList();

	cmd_list->IASetVertexBuffers(0, 1, &primitive_->GetVertexBufferView());
	cmd_list->IASetIndexBuffer(&primitive_->GetIndexBufferView());
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ワールド行列
	ID3D12DescriptorHeap* trans_heaps[] = { transform_cbv_heap_.Get() };
	cmd_list->SetDescriptorHeaps(1, trans_heaps);
	cmd_list->SetGraphicsRootDescriptorTable(7, transform_cbv_heap_->GetGPUDescriptorHandleForHeapStart());

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

	// 処理分ける必要ないかも
	/*switch (type_)
	{
			case kPlane:


					break;
			default:
					break;
	}*/
}

HRESULT PrimitiveComponent::CreateTransformResourceAndView()
{
	// GPUバッファの作成
	auto buffer_size = sizeof(DirectX::XMMATRIX);                                                // ワールド行列
	buffer_size = AligmentedValue(buffer_size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);  // 256の倍数にする

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);

	auto result = dx12_->GetDevice()->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(transform_const_buffer_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// マップ
	result = transform_const_buffer_->Map(0, nullptr, (void**)&world_matrix_);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// ビューの作成
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 1;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.NodeMask = 0;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = dx12_->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(transform_cbv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = transform_const_buffer_->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = buffer_size;

	dx12_->GetDevice()->CreateConstantBufferView(&cbv_desc, transform_cbv_heap_->GetCPUDescriptorHandleForHeapStart());

	return S_OK;
}
