#include "FBXComponent.h"

#include <d3d12.h>
#include <d3dx12.h>

#include "Actor.h"
#include "Game2.h"
#include "Helper.h"
#include "Renderer.h"
#include "Scene.h"
#include "XMFLOAT_Helper.h"

// "-" オペランド等が使えないため
using namespace DirectX;

FBXComponent::FBXComponent(Actor* owner, const char* file_name) : Component(owner),
fbx_model_(nullptr),
world_matrix_(nullptr)
{
	/*dx12_ = owner_->GetScene()->GetGame()->GetDx12();
	renderer_ = owner_->GetScene()->GetGame()->GetRenderer();*/

	renderer_->AddFBXComponent(this);

	fbx_model_ = dx12_->GetResourceManager()->LoadFBXModel(file_name);

	CreateTransformResourceAndView();

	for (auto& mesh : fbx_model_->mesh_data)
	{
		CalculateTangentAndBinormal(mesh);
	}
}

FBXComponent::~FBXComponent()
{
	renderer_->RemoveFBXComponent(this);
}

void FBXComponent::Update(float delta_time)
{
	// ワールド行列の更新
	auto pos = m_pOwner->GetPosition();
	auto rot = m_pOwner->GetRotation();
	auto scale = m_pOwner->GetScale();

	*world_matrix_ = DirectX::XMMatrixScaling(scale, scale, scale) * DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
}

void FBXComponent::ProcessInput(const InputState& state)
{
}

void FBXComponent::GenerateOutput()
{
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

		// ワールド行列
		ID3D12DescriptorHeap* trans_heaps[] = { transform_cbv_heap_.Get() };
		cmd_list->SetDescriptorHeaps(1, trans_heaps);
		cmd_list->SetGraphicsRootDescriptorTable(4, transform_cbv_heap_->GetGPUDescriptorHandleForHeapStart());

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

			// 法線マップ
			cmd_list->SetDescriptorHeaps(1, normal_map_srv_heap_.GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(5, normal_map_srv_heap_->GetGPUDescriptorHandleForHeapStart());

			// ARMマップ
			cmd_list->SetDescriptorHeaps(1, arm_map_srv_heap_.GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(6, arm_map_srv_heap_->GetGPUDescriptorHandleForHeapStart());

			cmd_list->DrawIndexedInstanced(mesh.indices.size(), 1, 0, 0, 0);
		}
	}
}

bool FBXComponent::CreateNormalMapAndView(const char* file_name)
{
	normal_map_resource_ = dx12_->GetResourceManager()->GetTextureFromPath(file_name);

	if (normal_map_resource_ == nullptr)
	{
		assert(0);
		return false;
	}

	// ディスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 1;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.NodeMask = 0;

	auto result = dx12_->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(normal_map_srv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// ディスクリプタの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = normal_map_resource_->GetDesc().Format;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	dx12_->GetDevice()->CreateShaderResourceView(normal_map_resource_.Get(), &srv_desc, normal_map_srv_heap_->GetCPUDescriptorHandleForHeapStart());

	return true;
}

bool FBXComponent::CreateArmMapAndView(const char* file_name)
{
	arm_map_resource_ = dx12_->GetResourceManager()->GetTextureFromPath(file_name);

	if (arm_map_resource_ == nullptr)
	{
		assert(0);
		return false;
	}

	// ディスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 1;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.NodeMask = 0;

	auto result = dx12_->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(arm_map_srv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// ディスクリプタの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = arm_map_resource_->GetDesc().Format;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	dx12_->GetDevice()->CreateShaderResourceView(arm_map_resource_.Get(), &srv_desc, arm_map_srv_heap_->GetCPUDescriptorHandleForHeapStart());

	return true;
}

HRESULT FBXComponent::CreateTransformResourceAndView()
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

HRESULT FBXComponent::CreateMaterialResourceAndView(std::string material_name)
{
	// マテリアルバッファの作成
	auto material_buffer_size = AligmentedValue(sizeof(FBXMaterial), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);  // 256の倍数にする

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(material_buffer_size);

	auto result = dx12_->GetDevice()->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(fbx_model_->material_const_buffer_table[material_name].ReleaseAndGetAddressOf()));

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

void FBXComponent::CalculateTangentAndBinormal(FBXMeshData& mesh_data)
{
	// MikkTSpaceアルゴリズムに基づいて計算

	// ポリゴン数を取得（インデックス数を3で割る）
	auto numPolygon = mesh_data.indices.size() / 3;

	for (auto polyNo = 0; polyNo < numPolygon; polyNo++)
	{
		auto no = polyNo * 3;
		auto& vert_0 = mesh_data.vertices[mesh_data.indices[no]];
		auto& vert_1 = mesh_data.vertices[mesh_data.indices[no + 1]];
		auto& vert_2 = mesh_data.vertices[mesh_data.indices[no + 2]];

		DirectX::XMFLOAT3 deltaPos1 = ToXMFLOAT3(ToXMVECTOR(vert_1.pos) - ToXMVECTOR(vert_0.pos));
		DirectX::XMFLOAT3 deltaPos2 = ToXMFLOAT3(ToXMVECTOR(vert_2.pos) - ToXMVECTOR(vert_0.pos));
		DirectX::XMFLOAT2 deltaUV1 = ToXMFLOAT2(ToXMVECTOR(vert_1.uv) - ToXMVECTOR(vert_0.uv));
		DirectX::XMFLOAT2 deltaUV2 = ToXMFLOAT2(ToXMVECTOR(vert_2.uv) - ToXMVECTOR(vert_0.uv));

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

		DirectX::XMFLOAT3 tangent = ToXMFLOAT3((ToXMVECTOR(deltaPos1) * deltaUV2.y - ToXMVECTOR(deltaPos2) * deltaUV1.y) * r);
		DirectX::XMFLOAT3 binormal = ToXMFLOAT3((ToXMVECTOR(deltaPos2) * deltaUV1.x - ToXMVECTOR(deltaPos1) * deltaUV2.x) * r);

		tangent = ToXMFLOAT3(ToNormalizeXMVECTOR(tangent));
		binormal = ToXMFLOAT3(ToNormalizeXMVECTOR(binormal));

		vert_0.tangent += tangent;
		vert_1.tangent += tangent;
		vert_2.tangent += tangent;

		vert_0.binormal += binormal;
		vert_1.binormal += binormal;
		vert_2.binormal += binormal;
	}

	// 接ベクトルと従法線ベクトルを正規化
	for (auto& vert : mesh_data.vertices)
	{
		vert.tangent = ToXMFLOAT3(ToNormalizeXMVECTOR(vert.tangent));
		vert.binormal = ToXMFLOAT3(ToNormalizeXMVECTOR(vert.binormal));
	}

	// 頂点バッファへの書き込み
	FBXVertex* vertex_map = nullptr;
	mesh_data.vertex_buffer->Map(0, nullptr, (void**)(&vertex_map));

	std::copy(mesh_data.vertices.begin(), mesh_data.vertices.end(), vertex_map);
	mesh_data.vertex_buffer->Unmap(0, nullptr);
}
