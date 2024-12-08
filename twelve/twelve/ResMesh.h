#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

struct ResMaterial
{
	DirectX::XMFLOAT3 Diffuse;			// 拡散反射成分
	DirectX::XMFLOAT3 Specular;			// 鏡面反射成分
	float Alpha;						// 透過成分
	float Shininess;					// 鏡面反射強度
	std::wstring DiffuseMap;			// ディフーズマップファイルパス
	std::wstring SpecularMap;			// スペキュラーマップファイルパス
	std::wstring ShininessMap;			// シャイネスマップファイルパス
	std::wstring NormalMap;				// 法線マップファイルパス

	DirectX::XMFLOAT4 BaseColor;		// ベースカラー
	float Metallic;						// 金属度
	float Roughness;					// 粗さ
	std::wstring BaseColorMap;			// ベースカラーマップファイルパス
	std::wstring MetallicRoughnessMap;	// 金属度・粗さマップファイルパス

	// TODO: 後で削除
	std::wstring MetallicMap;
	std::wstring RoughnessMap;
};

class MeshVertex
{
public:
	DirectX::XMFLOAT3 Position; // 位置
	DirectX::XMFLOAT3 Normal;   // 法線
	DirectX::XMFLOAT2 TexCoord; // テクスチャ座標
	DirectX::XMFLOAT3 Tangent;  // 接線

	MeshVertex() = default;

	MeshVertex(
		DirectX::XMFLOAT3 const& position,
		DirectX::XMFLOAT3 const& normal,
		DirectX::XMFLOAT2 const& texCoord,
		DirectX::XMFLOAT3 const& tangent)
		: Position(position)
		, Normal(normal)
		, TexCoord(texCoord)
		, Tangent(tangent)
	{
	}

	static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:
	static const int InputElementCount = 4;
	static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

struct ResMesh
{
	std::vector<MeshVertex> Vertices; // 頂点データ
	std::vector<uint32_t> Indices;    // インデックスデータ
	uint32_t MaterialId;              // マテリアル番号
};

/// <summary>
/// メッシュをロードする
/// </summary>
/// <param name="fileName">ファイルパス</param>
/// <param name="meshes">メッシュの格納先</param>
/// <param name="materials">マテリアルの格納先</param>
/// <returns></returns>
bool LoadMesh(
	const wchar_t* fileName,
	std::vector<ResMesh>& meshes,
	std::vector<ResMaterial>& materials);
