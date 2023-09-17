#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include "Dx12Wrapper.h"

struct PrimitiveVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
	DirectX::XMFLOAT3 color;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT3 binormal;
};

class Primitive
{
public:
	Primitive();
	~Primitive();

	virtual bool CreateVertexBuffer(ID3D12Device* device) = 0;
	virtual bool CreateIndexBuffer(ID3D12Device* device) = 0;
	virtual bool CreateTextureAndView(Dx12Wrapper* dx12, const char* file_name) = 0;
	virtual bool CreateNormalMapAndView(Dx12Wrapper* dx12, const char* file_name) = 0;
	virtual bool CreateArmMapAndView(Dx12Wrapper* dx12, const char* file_name) = 0;

	ID3D12Resource* GetVertexBuffer() const { return vertex_buffer_; }
	ID3D12Resource* GetIndexBuffer() const { return index_buffer_; }
	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertex_buffer_view_; }
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return index_buffer_view_; }

	ComPtr<ID3D12Resource> GetTextureResource() { return texture_resource_; }
	ComPtr<ID3D12DescriptorHeap> GetTextureSRVHeap() { return texture_srv_heap_; }

	ComPtr<ID3D12Resource> GetNormalMapResource() { return normal_map_resource_; }
	ComPtr<ID3D12DescriptorHeap> GetNormalMapSRVHeap() { return normal_map_srv_heap_; }

	ComPtr<ID3D12Resource> GetArmMapResource() { return arm_map_resource_; }
	ComPtr<ID3D12DescriptorHeap> GetArmMapSRVHeap() { return arm_map_srv_heap_; }

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

	// 法線マップ
	ComPtr<ID3D12Resource> normal_map_resource_;
	ComPtr<ID3D12DescriptorHeap> normal_map_srv_heap_;

	// ARMマップ
	ComPtr<ID3D12Resource> arm_map_resource_;
	ComPtr<ID3D12DescriptorHeap> arm_map_srv_heap_;

	unsigned int vertex_num_;
	unsigned int index_num_;
};