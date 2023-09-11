#include "Plane.h"
#include <d3dx12.h>
#include <cassert>

Plane::Plane()
{
}

Plane::~Plane()
{
}

bool Plane::CreateVertexBuffer(ID3D12Device* device)
{
	PlaneVertex vertices[] = {
		{ { -128.0f, -0.1f, -128.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }, // ����
		{ { 128.0f, -0.1f, -128.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },  // �E��
		{ { 128.0f, -0.1f, 128.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },   // �E��
		{ { -128.0f, -0.1f, 128.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },  // ����
	};

	vertex_num_ = sizeof(vertices) / sizeof(vertices[0]);

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices));

	auto result = device->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&buffer_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertex_buffer_)
	);

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// �o�b�t�@�ւ̏�������
	PlaneVertex* map = nullptr;
	result = vertex_buffer_->Map(0, nullptr, (void**)&map);
	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	std::copy(std::begin(vertices), std::end(vertices), map);

	vertex_buffer_->Unmap(0, nullptr);

	// ���_�o�b�t�@�r���[�̐ݒ�
	vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
	vertex_buffer_view_.SizeInBytes = sizeof(vertices);
	vertex_buffer_view_.StrideInBytes = sizeof(PlaneVertex);

	return true;
}

bool Plane::CreateIndexBuffer(ID3D12Device* device)
{
	unsigned short indices[] = {
		0, 1, 3,
		3, 1, 2,
	};

	index_num_ = sizeof(indices) / sizeof(indices[0]);

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices));

	auto result = device->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&buffer_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&index_buffer_)
	);

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// �o�b�t�@�ւ̏�������
	unsigned short* map = nullptr;
	result = index_buffer_->Map(0, nullptr, (void**)&map);
	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	std::copy(std::begin(indices), std::end(indices), map);

	index_buffer_->Unmap(0, nullptr);

	// �C���f�b�N�X�o�b�t�@�r���[�̐ݒ�
	index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
	index_buffer_view_.SizeInBytes = sizeof(indices);
	index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;

	return true;
}

bool Plane::CreateTextureAndView(Dx12Wrapper* dx12, const char* file_name)
{
	texture_resource_ = dx12->GetResourceManager()->GetTextureFromPath(file_name);

	if (texture_resource_ == nullptr)
	{
		assert(0);
		return false;
	}

	// �f�B�X�N���v�^�q�[�v�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 1;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.NodeMask = 0;

	auto result = dx12->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(texture_srv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// �f�B�X�N���v�^�̐ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = texture_resource_->GetDesc().Format;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	dx12->GetDevice()->CreateShaderResourceView(texture_resource_.Get(), &srv_desc, texture_srv_heap_->GetCPUDescriptorHandleForHeapStart());

	return true;
}

bool Plane::CreateNormalMapAndView(Dx12Wrapper* dx12, const char* file_name)
{
	normal_map_resource_ = dx12->GetResourceManager()->GetTextureFromPath(file_name);

	if (normal_map_resource_ == nullptr)
	{
		assert(0);
		return false;
	}

	// �f�B�X�N���v�^�q�[�v�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 1;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.NodeMask = 0;

	auto result = dx12->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(normal_map_srv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// �f�B�X�N���v�^�̐ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = normal_map_resource_->GetDesc().Format;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	dx12->GetDevice()->CreateShaderResourceView(normal_map_resource_.Get(), &srv_desc, normal_map_srv_heap_->GetCPUDescriptorHandleForHeapStart());

	return true;
}

bool Plane::CreateArmMapAndView(Dx12Wrapper* dx12, const char* file_name)
{
	arm_map_resource_ = dx12->GetResourceManager()->GetTextureFromPath(file_name);

	if (normal_map_resource_ == nullptr)
	{
		assert(0);
		return false;
	}

	// �f�B�X�N���v�^�q�[�v�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 1;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.NodeMask = 0;

	auto result = dx12->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(arm_map_srv_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// �f�B�X�N���v�^�̐ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = arm_map_resource_->GetDesc().Format;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	dx12->GetDevice()->CreateShaderResourceView(arm_map_resource_.Get(), &srv_desc, arm_map_srv_heap_->GetCPUDescriptorHandleForHeapStart());

	return true;
}
