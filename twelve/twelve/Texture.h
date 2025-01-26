#pragma once

#include <d3d12.h>
#include <ResourceUploadBatch.h>

#include "ComPtr.h"

class DescriptorHandle;
class DescriptorPool;

class Texture
{
public:
	Texture();
	~Texture();

	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		const wchar_t* filename,
		DirectX::ResourceUploadBatch& batch);

	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		const wchar_t* filename,
		bool isSRGB,
		DirectX::ResourceUploadBatch& batch);

	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		const D3D12_RESOURCE_DESC* pDesc,
		D3D12_RESOURCE_STATES initState,
		bool isCube);

	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		const D3D12_RESOURCE_DESC* pDesc,
		D3D12_RESOURCE_STATES initState,
		bool isCube,
		bool isSRGB);

	void Term();

	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const;
	ComPtr<ID3D12Resource>& GetComPtr();
	D3D12_RESOURCE_DESC GetDesc() const;

private:
	ComPtr<ID3D12Resource> m_pTex;
	DescriptorHandle* m_pHandle;
	DescriptorPool* m_pPool;

	Texture(const Texture&) = delete;
	void operator=(const Texture&) = delete;

	/// <summary>
	/// シェーダーリソースビューの設定を取得する
	/// </summary>
	/// <param name="isCube">キューブマップかどうか</param>
	/// <returns></returns>
	D3D12_SHADER_RESOURCE_VIEW_DESC GetViewDesc(bool isCube);
};
