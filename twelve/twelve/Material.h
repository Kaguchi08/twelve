#pragma once

#include <ResourceUploadBatch.h>
#include <map>

#include "DescriptorPool.h"
#include "Texture.h"
#include "ConstantBuffer.h"

class Material
{
public:
	enum TEXTURE_USAGE
	{
		TEXTURE_USAGE_DIFFUSE = 0,	// ディフューズマップとして利用
		TEXTURE_USAGE_SPECULAR,		// スペキュラマップとして利用
		TEXTURE_USAGE_SHININESS,	// シャイネスマップとして利用
		TEXTURE_USAGE_NORMAL,		// 法線マップとして利用

		TEXTURE_USAGE_BASE_COLOR,   // ベースカラーマップとして利用
		TEXTURE_USAGE_METALLIC,     // メタリックマップとして利用
		TEXTURE_USAGE_ROUGHNESS,    // ラフネスマップとして利用

		TEXTURE_USAGE_COUNT
	};

	Material();
	~Material();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="pPool">ディスクリプタプール</param>
	/// <param name="bufferSize">1マテリアル当たりの定数バッファのサイズ</param>
	/// <param name="count">マテリアル数</param>
	/// <returns></returns>
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		size_t bufferSize,
		size_t count);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	/// <summary>
	/// テクスチャを設定する
	/// </summary>
	/// <param name="index">マテリアル番号</param>
	/// <param name="usage">テクスチャの使用用途</param>
	/// <param name="path">テクスチャパス</param>
	/// <param name="batch">リソースアップロードバッチ</param>
	/// <returns></returns>
	bool SetTexture(
		size_t index,
		TEXTURE_USAGE usage,
		const std::wstring& path,
		DirectX::ResourceUploadBatch& batch);

	/// <summary>
	/// 定数バッファのポインタを取得する
	/// </summary>
	/// <param name="index">取得するマテリアル番号</param>
	/// <returns></returns>
	void* GetBufferPtr(size_t index) const;

	/// <summary>
	/// 定数バッファのポインタを取得する
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="index">マテリアル番号</param>
	/// <returns></returns>
	template<typename T>
	T* GetBufferPtr(size_t index) const
	{
		return reinterpret_cast<T*>(GetBufferPtr(index));
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetBufferAddress(size_t index) const;

	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(size_t index, TEXTURE_USAGE usage) const;

	size_t GetCount() const;

private:
	struct Subset
	{
		ConstantBuffer* pConstantBuffer; // 定数バッファ
		D3D12_GPU_DESCRIPTOR_HANDLE TextureHandle[TEXTURE_USAGE_COUNT]; // テクスチャハンドル
	};


	std::map<std::wstring, Texture*> m_pTextures; // テクスチャ
	std::vector<Subset> m_Subsets; // サブセット
	ID3D12Device* m_pDevice; // デバイス
	DescriptorPool* m_pPool; // ディスクリプタプール

	Material(const Material&) = delete;
	void operator=(const Material&) = delete;
};

constexpr auto TU_DIFFUSE = Material::TEXTURE_USAGE_DIFFUSE;
constexpr auto TU_SPECULAR = Material::TEXTURE_USAGE_SPECULAR;
constexpr auto TU_SHININESS = Material::TEXTURE_USAGE_SHININESS;
constexpr auto TU_NORMAL = Material::TEXTURE_USAGE_NORMAL;

constexpr auto TU_BASE_COLOR = Material::TEXTURE_USAGE_BASE_COLOR;
constexpr auto TU_METALLIC = Material::TEXTURE_USAGE_METALLIC;
constexpr auto TU_ROUGHNESS = Material::TEXTURE_USAGE_ROUGHNESS;
