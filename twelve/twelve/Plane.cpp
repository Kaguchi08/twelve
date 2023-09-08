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
		{ { -0.5f, 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } }, // ����
		{ { 0.5f, 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },  // �E��
		{ { 0.5f, 0.0f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },   // �E��
		{ { -0.5f, 0.0f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },  // ����
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
