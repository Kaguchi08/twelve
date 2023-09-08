#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include "XMFLOAT_Helper.h"

struct PlaneVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
};

class Plane
{
public:
	Plane();
	~Plane();

	bool CreateVertexBuffer(ID3D12Device* device);
	bool CreateIndexBuffer(ID3D12Device* device);

	ID3D12Resource* GetVertexBuffer() const { return vertex_buffer_; }
	ID3D12Resource* GetIndexBuffer() const { return index_buffer_; }
	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertex_buffer_view_; }
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return index_buffer_view_; }

private:
	ID3D12Resource* vertex_buffer_;
	ID3D12Resource* index_buffer_;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_;
	D3D12_INDEX_BUFFER_VIEW index_buffer_view_;
};