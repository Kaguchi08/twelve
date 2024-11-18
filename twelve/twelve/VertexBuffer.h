#pragma once

#include <d3d12.h>

#include "ComPtr.h"

class VertexBuffer
{
public:
	VertexBuffer();
	~VertexBuffer();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="size">頂点バッファサイズ</param>
	/// <param name="stride">1頂点当たりのサイズ</param>
	/// <param name="pInitData">初期化データ</param>
	/// <returns></returns>
	bool Init(
		ID3D12Device* pDevice,
		size_t size,
		size_t stride,
		const void* pInitData = nullptr);

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="pDevice">デバイス</param>
	/// <param name="size">頂点バッファサイズ</param>
	/// <param name="pInitData">初期化データ</param>
	/// <returns></returns>
	template<typename T>
	bool Init(
		ID3D12Device* pDevice,
		size_t size,
		const T* pInitData = nullptr)
	{
		return Init(pDevice, size, sizeof(T), pInitData);
	}

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	/// <summary>
	/// メモリマッピングを行う
	/// </summary>
	/// <returns></returns>
	void* Map();

	/// <summary>
	/// メモリマッピングを解除する
	/// </summary>
	void Unmap();

	/// <summary>
	/// メモリマッピングを行う
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <returns></returns>
	template<typename T>
	T* Map() const
	{
		return reinterpret_cast<T*>(Map());
	}

	D3D12_VERTEX_BUFFER_VIEW GetView() const;

private:
	ComPtr<ID3D12Resource> m_pBuffer; // 頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW m_View; // 頂点バッファビュー

	VertexBuffer(const VertexBuffer&) = delete;
	void operator=(const VertexBuffer&) = delete;
};
