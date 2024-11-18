#pragma once

#include <d3d12.h>
#include <cstdint>
#include <vector>

#include "ComPtr.h"

class CommandList
{
public:
	CommandList();
	~CommandList();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="type">コマンドリストタイプ</param>
	/// <param name="count">アロケータの数</param>
	/// <returns></returns>
	bool Init(
		ID3D12Device* pDevice,
		D3D12_COMMAND_LIST_TYPE type,
		uint32_t count);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	/// <summary>
	/// リセット処理を行ったコマンドリストを取得する
	/// </summary>
	/// <returns></returns>
	ID3D12GraphicsCommandList* Reset();

private:
	ComPtr<ID3D12GraphicsCommandList> m_pCmdList; // コマンドリスト
	std::vector<ComPtr<ID3D12CommandAllocator>> m_pAllocators; // コマンドアロケータ
	uint32_t m_Index; // アロケータ番号

	CommandList(const CommandList&) = delete;
	void operator=(const CommandList&) = delete;
};
