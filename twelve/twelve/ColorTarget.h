#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>

#include "ComPtr.h"

class DescriptorHandle;
class DescriptorPool;

class ColorTarget
{
public:
	ColorTarget();
	~ColorTarget();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="pPoolRTV">ディスクリプタプール(RTV)</param>
	/// <param name="pPoolSRV">ディスクリプタプール(SRV)</param>
	/// <param name="width">横幅</param>
	/// <param name="height">縦幅</param>
	/// <param name="format">ピクセルフォーマット</param>
	/// <param name="clearColor">クリアカラー</param>
	/// <returns></returns>
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPoolRTV,
		DescriptorPool* pPoolSRV,
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format,
		float clearColor[4]);

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
		DescriptorPool* pPoolRTV,
		bool useSRGB,
		uint32_t index,
		IDXGISwapChain* pSwapChain);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	/// <summary>
	/// ビューをクリアする
	/// </summary>
	/// <param name="pCmdList">コマンドリスト</param>
	void ClearView(ID3D12GraphicsCommandList* pCmdList);

	DescriptorHandle* GetHandleRTV() const;
	DescriptorHandle* GetHandleSRV() const;
	ID3D12Resource* GetResource() const;
	D3D12_RESOURCE_DESC GetDesc() const;
	D3D12_RENDER_TARGET_VIEW_DESC GetRTVDesc() const;
	D3D12_SHADER_RESOURCE_VIEW_DESC GetSRVDesc() const;

private:
	ComPtr<ID3D12Resource> m_pTarget; // レンダーターゲット
	DescriptorHandle* m_pHandleRTV; // ディスクリプタハンドル(RTV)
	DescriptorHandle* m_pHandleSRV; // ディスクリプタハンドル(SRV)
	DescriptorPool* m_pPoolRTV; // ディスクリプタプール(RTV)
	DescriptorPool* m_pPoolSRV; // ディスクリプタプール(SRV)
	D3D12_RENDER_TARGET_VIEW_DESC m_RTVDesc; // レンダーターゲットビューの構成設定
	D3D12_SHADER_RESOURCE_VIEW_DESC m_SRVDesc; // シェーダーリソースビューの構成設定
	float m_ClearColor[4]; // クリアカラー

	ColorTarget(const ColorTarget&) = delete;
	void operator=(const ColorTarget&) = delete;
};
