#pragma once

#include <d3d12.h>
#include <vector>

#include "ComPtr.h"

class DescriptorHandle;
class DescriptorPool;

class ConstantBuffer
{
public:
	ConstantBuffer();
	~ConstantBuffer();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="pPool">ディスクリプタプール</param>
	/// <param name="size">サイズ</param>
	/// <returns></returns>
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		size_t size);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const;
	void* GetPtr() const;
	template<typename T>
	T* GetPtr() const
	{
		return reinterpret_cast<T*>(GetPtr());
	}

private:
	ComPtr<ID3D12Resource> m_pCB; // 定数バッファ
	DescriptorHandle* m_pHandle; // ディスクリプタハンドル
	DescriptorPool* m_pPool; // ディスクリプタプール
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_Desc; // 定数バッファビューの構成設定
	void* m_pMappedPtr; // マップされたポインタ

	ConstantBuffer(const ConstantBuffer&) = delete;
	void operator=(const ConstantBuffer&) = delete;
};
