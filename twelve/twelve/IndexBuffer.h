#pragma once

#include <d3d12.h>
#include <cstdint>

#include "ComPtr.h"

class IndexBuffer
{
public:
	IndexBuffer();
	~IndexBuffer();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="count">インデックス数</param>
	/// <param name="pInitData">初期化データ</param>
	/// <returns></returns>
	bool Init(
		ID3D12Device* pDevice,
		uint32_t count,
		const uint32_t* pInitData = nullptr);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	/// <summary>
	/// メモリマッピングを行う
	/// </summary>
	/// <returns></returns>
	uint32_t* Map();

	/// <summary>
	/// メモリマッピングを解除する
	/// </summary>
	void Unmap();

	D3D12_INDEX_BUFFER_VIEW GetView() const;
	size_t GetCount() const;

private:
	ComPtr<ID3D12Resource> m_pBuffer; // インデックスバッファ
	D3D12_INDEX_BUFFER_VIEW m_View; // インデックスバッファビュー
	size_t m_Count; // インデックス数

	IndexBuffer(const IndexBuffer&) = delete;
	void operator=(const IndexBuffer&) = delete;
};
