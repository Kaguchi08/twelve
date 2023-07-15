#include "ModelComponent.h"
#include "Renderer.h"
#include "Game.h"
#include "Actor.h"
#include "Scene.h"
#include "Helper.h"
#include <d3dx12.h>
#include <d3d12.h>

ModelComponent::ModelComponent(Actor* owner, const char* file_name, int draw_order) :
    Component(owner, draw_order)
{
    dx12_ = owner->GetScene()->GetGame()->GetDx12();
	renderer_ = owner->GetScene()->GetGame()->GetRenderer();

	renderer_->AddModelComponent(this);

    // モデル
    model_ = dx12_->LoadModel(file_name);
    bone_matrices_.resize(model_->bone_name_array.size());
    std::fill(bone_matrices_.begin(), bone_matrices_.end(), DirectX::XMMatrixIdentity());

    // 座標変換用リソース作成
    auto result = CreateTransformResourceAndView();

    if (FAILED(result)) {
        assert(0);
	}
}

ModelComponent::~ModelComponent()
{
	renderer_->RemoveModelComponent(this);
}

void ModelComponent::Update(float delta_time)
{
}

void ModelComponent::Draw()
{
	auto cmd_list = dx12_->GetCommandList();
	cmd_list->IASetVertexBuffers(0, 1, &model_->vb_view);
	cmd_list->IASetIndexBuffer(&model_->ib_view);
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* trans_heaps[] = { transform_heap_.Get() };
	cmd_list->SetDescriptorHeaps(1, trans_heaps);
	cmd_list->SetGraphicsRootDescriptorTable(1, transform_heap_->GetGPUDescriptorHandleForHeapStart());

	auto material_heap = model_->material_heap.Get();
	ID3D12DescriptorHeap* material_heaps[] = { material_heap };
	cmd_list->SetDescriptorHeaps(1, material_heaps);

	// マテリアルの描画
	auto handle = material_heap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	auto inc_size = dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	for (auto& material : model_->materials)
	{
		cmd_list->SetGraphicsRootDescriptorTable(2, handle);
		cmd_list->DrawIndexedInstanced(material.indices_num, 2, idxOffset, 0, 0);
		handle.ptr += inc_size;
		idxOffset += material.indices_num;
	}
}

HRESULT ModelComponent::CreateTransformResourceAndView()
{
	// GPUバッファの作成
	auto buffer_size = sizeof(DirectX::XMMATRIX) * (1 + bone_matrices_.size()); // wordl行列 + ボーン行列 * ボーン数
	buffer_size = AligmentedValue(buffer_size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT); // 256の倍数にする

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);

	auto result = dx12_->GetDevice()->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(transform_buffer_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// マップ
	result = transform_buffer_->Map(0, nullptr, (void**)&mapped_matrices_);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// ワールド行列
	mapped_matrices_[0] = DirectX::XMMatrixIdentity();
	// ボーン行列
	std::copy(bone_matrices_.begin(), bone_matrices_.end(), mapped_matrices_ + 1);

	// ビューの作成
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 1;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.NodeMask = 0;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = dx12_->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(transform_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = transform_buffer_->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = buffer_size;

	dx12_->GetDevice()->CreateConstantBufferView(&cbv_desc, transform_heap_->GetCPUDescriptorHandleForHeapStart());

	return S_OK;
}
