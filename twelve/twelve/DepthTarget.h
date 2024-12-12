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
	/// <param name="pPoolDSV">ディスクリプタプール(DSC)</param>
	/// <param name="pPoolDSV">ディスクリプタプール(SRV)</param>
	/// <param name="width">横幅</param>
	/// <param name="height">縦幅</param>
	/// <param name="format">ピクセルフォーマット</param>
	/// <param name="clearDepth">クリア深度値</param>
	/// <param name="clearStencil">クリアステンシル値</param>
	/// <returns></returns>
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPoolDSV,
		DescriptorPool* pPoolSRV,
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format,
		float clearDepth,
		uint8_t clearStencil);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	/// <summary>
	/// ビューをクリアする
	/// </summary>
	/// <param name="pCmdList">コマンドリスト</param>
	void ClearView(ID3D12GraphicsCommandList* pCmdList);

	DescriptorHandle* GetHandleDSV() const;
	DescriptorHandle* GetHandleSRV() const;
	ID3D12Resource* GetResource() const;
	D3D12_RESOURCE_DESC GetDesc() const;
	D3D12_DEPTH_STENCIL_VIEW_DESC GetDSVDesc() const;
	D3D12_SHADER_RESOURCE_VIEW_DESC GetSRVDesc() const;

private:
	ComPtr<ID3D12Resource> m_pTarget; // 深度ステンシルバッファ
	DescriptorHandle* m_pHandleDSV; // ディスクリプタハンドル(DSV)
	DescriptorHandle* m_pHandleSRV; // ディスクリプタハンドル(SRV)
	DescriptorPool* m_pPoolDSV; // ディスクリプタプール(DSV)
	DescriptorPool* m_pPoolSRV; // ディスクリプタプール(SRV)
	D3D12_DEPTH_STENCIL_VIEW_DESC m_DSVDesc; // 深度ステンシルビューの構成設定
	D3D12_SHADER_RESOURCE_VIEW_DESC m_SRVDesc; // シェーダリソースビューの構成設定
	float m_ClearDepth; // クリア深度値
	uint8_t m_ClearStencil; // クリアステンシル値

	DepthTarget(const DepthTarget&) = delete;
	void operator=(const DepthTarget&) = delete;
};
