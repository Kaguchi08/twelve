#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>

#include "ComPtr.h"

class DescriptorHandle;
class DescriptorPool;

class RenderTarget
{
public:
	RenderTarget();
	~RenderTarget();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="pPool">ディスクリプタプール</param>
	/// <param name="width">横幅</param>
	/// <param name="height">縦幅</param>
	/// <param name="format">ピクセルフォーマット</param>
	/// <param name="useSRGB">sRGBフォーマットを使用するか</param>
	/// <returns></returns>
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format,
		bool useSRGB);

	/// <summary>
	/// 初期化処理（バックバッファから）
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="pPool">ディスクリプタプール</param>
	/// <param name="useSRGB">sRGBフォーマットを使用するか</param>
	/// <param name="index">バックバッファ番号</param>
	/// <param name="pSwapChain">スワップチェイン</param>
	/// <returns></returns>
	bool InitFromBackBuffer(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		bool useSRGB,
		uint32_t index,
		IDXGISwapChain* pSwapChain);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	DescriptorHandle* GetHandle() const;
	ID3D12Resource* GetResource() const;
	D3D12_RESOURCE_DESC GetDesc() const;
	D3D12_RENDER_TARGET_VIEW_DESC GetViewDesc() const;

private:
	ComPtr<ID3D12Resource> m_pTarget; // レンダーターゲット
	DescriptorHandle* m_pHandle; // ディスクリプタハンドル
	DescriptorPool* m_pPool; // ディスクリプタプール
	D3D12_RENDER_TARGET_VIEW_DESC m_Desc; // レンダーターゲットビューの構成設定

	RenderTarget(const RenderTarget&) = delete;
	void operator=(const RenderTarget&) = delete;
};
