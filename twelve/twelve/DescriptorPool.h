#pragma once

#include <d3d12.h>
#include <atomic>

#include "ComPtr.h"
#include "Pool.h"

class DescriptorHandle
{
public:
	D3D12_CPU_DESCRIPTOR_HANDLE HandleCPU; // CPUディスクリプタハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE HandleGPU; // GPUディスクリプタハンドル

	bool HasCPU() const { return HandleCPU.ptr != 0; }
	bool HasGPU() const { return HandleGPU.ptr != 0; }
};

class DescriptorPool
{
public:
	/// <summary>
	/// 生成処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="pDesc">ディスクリプタヒープの構成設定</param>
	/// <param name="ppPool">ディスクリプタプールの格納先</param>
	/// <returns></returns>
	static bool Create(
		ID3D12Device* pDevice,
		const D3D12_DESCRIPTOR_HEAP_DESC* pDesc,
		DescriptorPool** ppPool);

	/// <summary>
	/// 参照カウントを増やす
	/// </summary>
	void AddRef();

	/// <summary>
	/// 解放処理
	/// </summary>
	void Release();

	/// <summary>
	/// 参照カウントを取得する
	/// </summary>
	/// <returns></returns>
	uint32_t GetCount() const;

	/// <summary>
	/// ディスクリプタハンドルを割り当てる
	/// </summary>
	/// <returns>割り当てられたディスクリプタハンドル</returns>
	DescriptorHandle* AllocHandle();

	/// <summary>
	/// ディスクリプタハンドルを解放する
	/// </summary>
	/// <param name="pHandle">解放するハンドルのポインタ</param>
	void FreeHandle(DescriptorHandle*& pHandle);

	/// <summary>
	/// 利用可能なハンドル数を取得する
	/// </summary>
	/// <returns></returns>
	uint32_t GetAvailableHandleCount() const;

	/// <summary>
	/// 割り当て済みのハンドル数を取得する
	/// </summary>
	/// <returns></returns>
	uint32_t GetAllocatedHandleCount() const;

	/// <summary>
	/// ハンドル総数を取得する
	/// </summary>
	/// <returns></returns>
	uint32_t GetHandleCount() const;

	/// <summary>
	/// ディスクリプタヒープを取得する
	/// </summary>
	/// <returns></returns>
	ID3D12DescriptorHeap* const GetHeap() const;

private:
	std::atomic<uint32_t> m_RefCount; // 参照カウント
	Pool<DescriptorHandle> m_Pool; // ディスクリプタハンドルのプール
	ComPtr<ID3D12DescriptorHeap> m_pHeap; // ディスクリプタヒープ
	uint32_t m_DescriptorSize; // ディスクリプタサイズ

	DescriptorPool();
	~DescriptorPool();

	DescriptorPool(const DescriptorPool&) = delete;
	void operator=(const DescriptorPool&) = delete;
};
