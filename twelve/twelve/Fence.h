#pragma once

#include <d3d12.h>

#include "ComPtr.h"

class Fence
{
public:
	Fence();
	~Fence();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <returns></returns>
	bool Init(ID3D12Device* pDevice);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	/// <summary>
	/// シグナル状態になるまで指定された時間待機する
	/// </summary>
	/// <param name="pQueue">コマンドキュー</param>
	/// <param name="timeout">タイムアウト時間(ミリ秒)</param>
	void Wait(ID3D12CommandQueue* pQueue, UINT timeout);

	/// <summary>
	/// シグナル状態になるまで待機する
	/// </summary>
	/// <param name="pQueue"></param>
	void Sync(ID3D12CommandQueue* pQueue);

private:
	ComPtr<ID3D12Fence> m_pFence; // フェンス
	HANDLE m_Event; // イベント
	UINT m_Counter; // カウンター

	Fence(const Fence&) = delete;
	void operator=(const Fence&) = delete;
};
