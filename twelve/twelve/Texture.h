#pragma once

#include <d3d12.h>
#include <ResourceUploadBatch.h>

#include "ComPtr.h"

class DescriptorHandle;
class DescriptorPool;

class Texture
{
public:
	Texture();
	~Texture();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="pPool">ディスクリプタプール</param>
	/// <param name="filename">ファイル名</param>
	/// <param name="isSRGB">sRGBフォーマットにするか</param>
	/// <param name="batch">更新バッチ</param>
	/// <returns></returns>
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		const wchar_t* filename,
		bool isSRGB,
		DirectX::ResourceUploadBatch& batch);

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="pPool">ディスクリプタプール</param>
	/// <param name="pDesc">構成設定</param>
	/// <param name="isSRGB">sRGBフォーマットにするか</param>
	/// <param name="isCube">キューブマップかどうか</param>
	/// <returns></returns>
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		const D3D12_RESOURCE_DESC* pDesc,
		bool isSRGB,
		bool isCube);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const;

private:
	ComPtr<ID3D12Resource> m_pTex;
	DescriptorHandle* m_pHandle;
	DescriptorPool* m_pPool;

	Texture(const Texture&) = delete;
	void operator=(const Texture&) = delete;

	/// <summary>
	/// シェーダーリソースビューの設定を取得する
	/// </summary>
	/// <param name="isCube">キューブマップかどうか</param>
	/// <returns></returns>
	D3D12_SHADER_RESOURCE_VIEW_DESC GetViewDesc(bool isCube);
};
