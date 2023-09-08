#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include "Dx12Wrapper.h"

class Primitive
{
public:
	Primitive();
	~Primitive();

	virtual bool CreateVertexBuffer(ID3D12Device* device) = 0;
	virtual bool CreateIndexBuffer(ID3D12Device* device) = 0;
	virtual bool CreateTextureAndView(Dx12Wrapper* dx12, const char* file_name) = 0;

	ID3D12Resource* GetVertexBuffer() const { return vertex_buffer_; }
	ID3D12Resource* GetIndexBuffer() const { return index_buffer_; }
	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertex_buffer_view_; }
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return index_buffer_view_; }

	ComPtr<ID3D12Resource> GetTextureResource() { return texture_resource_; }
	ComPtr<ID3D12DescriptorHeap> GetTextureSRVHeap() { return texture_srv_heap_; }

	unsigned int GetVertexNum() const { return vertex_num_; }
	unsigned int GetIndexNum() const { return index_num_; }

protected:
	ID3D12Resource* vertex_buffer_;
	ID3D12Resource* index_buffer_;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_;
	D3D12_INDEX_BUFFER_VIEW index_buffer_view_;

	// テクスチャ
	ComPtr<ID3D12Resource> texture_resource_;
	ComPtr<ID3D12DescriptorHeap> texture_srv_heap_;

	unsigned int vertex_num_;
	unsigned int index_num_;
};