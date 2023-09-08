#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

class Primitive
{
public:
	Primitive();
	~Primitive();

	virtual bool CreateVertexBuffer(ID3D12Device* device) = 0;
	virtual bool CreateIndexBuffer(ID3D12Device* device) = 0;

	ID3D12Resource* GetVertexBuffer() const { return vertex_buffer_; }
	ID3D12Resource* GetIndexBuffer() const { return index_buffer_; }
	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertex_buffer_view_; }
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return index_buffer_view_; }
	unsigned int GetVertexNum() const { return vertex_num_; }
	unsigned int GetIndexNum() const { return index_num_; }

protected:
	ID3D12Resource* vertex_buffer_;
	ID3D12Resource* index_buffer_;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_;
	D3D12_INDEX_BUFFER_VIEW index_buffer_view_;

	unsigned int vertex_num_;
	unsigned int index_num_;
};