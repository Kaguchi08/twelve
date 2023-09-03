#pragma once
#include <wrl.h>
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <memory>

using Microsoft::WRL::ComPtr;

// 構造体

enum ModelType
{
	PMD,
	FBX,
};

// シェーダー側に投げられるマテリアルデータ
struct MaterialForHlsl
{
	DirectX::XMFLOAT3 diffuse;
	float alpha;
	DirectX::XMFLOAT3 specular;
	float specularity;
	DirectX::XMFLOAT3 ambient;
};

// その他のマテリアルデータ
struct AdditionalMaterial
{
	std::string tex_path;
	int toon_idx;
	bool edge_flg;
};

// 全体をまとめたデータ
struct Material
{
	unsigned int indices_num;
	MaterialForHlsl hlsl;
	AdditionalMaterial additional;
};

struct BoneNode
{
	uint32_t bone_idx;
	uint32_t bone_type;
	uint32_t parent_bone;
	uint32_t ik_parent_bone;
	DirectX::XMFLOAT3 start_pos;
	std::vector<BoneNode*> children;
};

struct PMDIK
{
	uint16_t bone_idx;
	uint16_t target_bone_idx;
	uint16_t iterations;
	float limit;
	std::vector<uint16_t> node_idxes;;
};

// モーション関連
struct KeyFrame
{
	unsigned int frame_no;
	DirectX::XMVECTOR quaternion;
	DirectX::XMFLOAT3 offset;
	DirectX::XMFLOAT2 p1, p2;

	KeyFrame(unsigned int fno,
			 const DirectX::XMVECTOR& q,
			 const DirectX::XMFLOAT3& ofst,
			 const DirectX::XMFLOAT2& ip1,
			 const DirectX::XMFLOAT2& ip2) :
		frame_no(fno),
		quaternion(q),
		offset(ofst),
		p1(ip1),
		p2(ip2)
	{
	}
};

//IKオンオフデータ
struct VMDIKEnable
{
	uint32_t frame_no;
	std::unordered_map<std::string, bool> ik_enable_table;
};

// FBXモデルの頂点データ
struct FBXVertex
{
	float pos[3]; // x, y, z // 座標
	float normal[3]; // nx, ny, nz // 法線ベクトル
	float uv[2]; // u, v // UV座標
};

// PMDモデル情報をまとめた構造体
struct PMDModel
{
	// 全体のインデックス数
	unsigned int indices_num;
	// 頂点配列
	std::vector<DirectX::XMFLOAT3> vertices;

	// マテリアル関連
	std::vector<Material> materials;
	ComPtr<ID3D12Resource> material_const_buffer = nullptr;
	ComPtr<ID3D12DescriptorHeap> material_cbv_heap = nullptr;
	std::vector<ComPtr<ID3D12Resource>> texture_resources;
	std::vector<ComPtr<ID3D12Resource>> sph_resources;
	std::vector<ComPtr<ID3D12Resource>> spa_resources;
	std::vector<ComPtr<ID3D12Resource>> toon_resources;

	// 頂点関連
	ComPtr<ID3D12Resource> vertex_buffer = nullptr;
	ComPtr<ID3D12Resource> index_buffer = nullptr;

	// ビュー
	D3D12_VERTEX_BUFFER_VIEW vb_view = {};
	D3D12_INDEX_BUFFER_VIEW ib_view = {};

	// ボーン情報
	std::unordered_map<std::string, BoneNode> bone_node_table;
	std::vector<std::string> bone_name_array;
	std::vector<BoneNode*> bone_node_address_array;
	std::vector<PMDIK> ik_data;
	std::vector<uint32_t> knee_idxes;
};

struct FBXMeshData
{
	ComPtr<ID3D12Resource> vertex_buffer;
	ComPtr<ID3D12Resource> index_buffer;
	D3D12_VERTEX_BUFFER_VIEW vb_view;
	D3D12_INDEX_BUFFER_VIEW ib_view;
	std::vector<FBXVertex> vertices;
	std::vector<unsigned int> indices;
	std::string material_name;
};

// FBXモデル情報をまとめた構造体
struct FBXModel
{
	// 全体のインデックス数
	unsigned int indices_num;

	// メッシュデータ
	std::vector<FBXMeshData> mesh_data;
};

struct VMDAnimation
{
	std::unordered_map<std::string, std::vector<KeyFrame>> motion_table;
	std::vector<VMDIKEnable> ik_enable_table;
	unsigned int duration;
};

struct Animation
{
	std::shared_ptr<VMDAnimation> vmd_anim;
	bool is_loop;
};