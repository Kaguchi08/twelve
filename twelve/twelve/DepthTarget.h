#pragma once

#include <d3d12.h>
#include <cstdint>

#include "ComPtr.h"

class DescriptorHandle;
class DescriptorPool;

class DepthTarget
{
public:
	DepthTarget();
	~DepthTarget();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="pPool">ディスクリプタプール</param>
	/// <param name="width">横幅</param>
	/// <param name="height">縦幅</param>
	/// <param name="format">ピクセルフォーマット</param>
	/// <returns></returns>
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	DescriptorHandle* GetHandle() const;
	ID3D12Resource* GetResource() const;
	D3D12_RESOURCE_DESC GetDesc() const;
	D3D12_DEPTH_STENCIL_VIEW_DESC GetViewDesc() const;
private:
	ComPtr<ID3D12Resource> m_pTarget; // 深度ステンシルバッファ
	DescriptorHandle* m_pHandle; // ディスクリプタハンドル
	DescriptorPool* m_pPool; // ディスクリプタプール
	D3D12_DEPTH_STENCIL_VIEW_DESC m_Desc; // 深度ステンシルビューの構成設定

	DepthTarget(const DepthTarget&) = delete;
	void operator=(const DepthTarget&) = delete;
};
