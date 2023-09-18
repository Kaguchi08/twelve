#include "FBXComponent.h"
#include "Renderer.h"
#include "Game.h"
#include "Actor.h"
#include "Scene.h"
#include "Helper.h"
#include "XMFLOAT_Helper.h"
#include <d3dx12.h>
#include <d3d12.h>

FBXComponent::FBXComponent(Actor* owner, const char* file_name, int draw_order) :
	Component(owner, draw_order),
	fbx_model_(nullptr),
	world_matrix_(nullptr)
{
	dx12_ = owner_->GetScene()->GetGame()->GetDx12();
	renderer_ = owner_->GetScene()->GetGame()->GetRenderer();

	renderer_->AddFBXComponent(this);

	fbx_model_ = dx12_->GetResourceManager()->LoadFBXModel(file_name);

	CreateTransformResourceAndView();
}

FBXComponent::~FBXComponent()
{
	renderer_->RemoveFBXComponent(this);
}

void FBXComponent::Update(float delta_time)
{
	// ワールド行列の更新
	auto pos = owner_->GetPosition();
	auto rot = owner_->GetRotation();

	*world_matrix_ = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z)
		* DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
}

void FBXComponent::Draw(bool is_shadow)
{
	for (auto& mesh : fbx_model_->mesh_data)
	{
		auto cmd_list = dx12_->GetCommandList();
		cmd_list->IASetVertexBuffers(0, 1, &mesh.vb_view);
		cmd_list->IASetIndexBuffer(&mesh.ib_view);
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// ここでバッファをセット
		auto result = CreateMaterialResourceAndView(mesh.material_name);
		if (FAILED(result))
		{
			assert(0);
		}

		// テクスチャをセット
		result = CreateTextureView(mesh.material_name);
		if (FAILED(result))
		{
			assert(0);
		}

		if (is_shadow)
		{
			cmd_list->DrawIndexedInstanced(mesh.indices.size(), 1, 0, 0, 0);
		}
		else
		{
			ID3D12DescriptorHeap* material_heaps[] = { fbx_model_->material_cbv_heap_table[mesh.material_name].Get() };
			cmd_list->SetDescriptorHeaps(1, material_heaps);
			cmd_list->SetGraphicsRootDescriptorTable(1, fbx_model_->material_cbv_heap_table[mesh.material_name]->GetGPUDescriptorHandleForHeapStart());

			ID3D12DescriptorHeap* texture_heaps[] = { fbx_model_->texture_srv_heap_table[mesh.material_name].Get() };
			cmd_list->SetDescriptorHeaps(1, texture_heaps);
			cmd_list->SetGraphicsRootDescriptorTable(3, fbx_model_->texture_srv_heap_table[mesh.material_name]->GetGPUDescriptorHandleForHeapStart());
			cmd_list->DrawIndexedInstanced(mesh.indices.size(), 1, 0, 0, 0);
		}
	}
}

HRESULT FBXComponent::CreateTransformResourceAndView()
{
	// GPUバッファの作成
	auto buffer_size = sizeof(DirectX::XMMATRIX); // ワールド行列
	buffer_size = AligmentedValue(buffer_size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT); // 256の倍数にする

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);

	auto result = dx12_->GetDevice()->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(transform_const_buffer_.ReleaseAndGetAddressOf())
	);

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

HRESULT FBXComponent::CreateMaterialResourceAndView(std::string material_name)
{
	// マテリアルバッファの作成
	auto material_buffer_size = AligmentedValue(sizeof(FBXMaterial), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT); // 256の倍数にする

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(material_buffer_size);

	auto result = dx12_->GetDevice()->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(fbx_model_->material_const_buffer_table[material_name].ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// マテリアルバッファのマップ
	char* mapped_material = nullptr;

	result = fbx_model_->material_const_buffer_table[material_name]->Map(0, nullptr, (void**)&mapped_material);
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// マテリアルバッファへの書き込み
	*((FBXMaterial*)mapped_material) = fbx_model_->material_table[material_name];

	// マテリアルバッファのアンマップ
	fbx_model_->material_const_buffer_table[material_name]->Unmap(0, nullptr);

	// ディスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 1;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.NodeMask = 0;

	result = dx12_->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(fbx_model_->material_cbv_heap_table[material_name].ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// View 作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = fbx_model_->material_const_buffer_table[material_name]->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = material_buffer_size;

	auto handle = fbx_model_->material_cbv_heap_table[material_name]->GetCPUDescriptorHandleForHeapStart();
	dx12_->GetDevice()->CreateConstantBufferView(&cbv_desc, handle);

	return S_OK;
}

HRESULT FBXComponent::CreateTextureView(std::string material_name)
{
	auto texture_resource_ = fbx_model_->texture_resource_table[material_name];

	if (texture_resource_ == nullptr)
	{
		assert(0);
		return S_FALSE;
	}

	// ディスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 1;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.NodeMask = 0;

	auto result = dx12_->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(fbx_model_->texture_srv_heap_table[material_name].ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// ディスクリプタの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = texture_resource_->GetDesc().Format;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	dx12_->GetDevice()->CreateShaderResourceView(texture_resource_.Get(), &srv_desc, fbx_model_->texture_srv_heap_table[material_name]->GetCPUDescriptorHandleForHeapStart());

	return S_OK;
}
