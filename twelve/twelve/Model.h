#pragma once
#include <wrl.h>
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

// �\����

// �V�F�[�_�[���ɓ�������}�e���A���f�[�^
struct MaterialForHlsl
{
	DirectX::XMFLOAT3 diffuse;
	float alpha;
	DirectX::XMFLOAT3 specular;
	float specularity;
	DirectX::XMFLOAT3 ambient;
};

// ���̑��̃}�e���A���f�[�^
struct AdditionalMaterial
{
	std::string texPath;
	int toonIdx;
	bool edgeFlg;
};

// �S�̂��܂Ƃ߂��f�[�^
struct Material
{
	unsigned int indicesNum;
	MaterialForHlsl hlsl;
	AdditionalMaterial additional;
};

struct BoneNode
{
	uint32_t boneIdx;
	uint32_t boneType;
	uint32_t parentBone;
	uint32_t ikParentBone;
	DirectX::XMFLOAT3 startPos;
	std::vector<BoneNode*> children;
};

struct PMDIK
{
	uint16_t boneIdx;
	uint16_t targetBoneIdx;
	uint16_t iterations;
	float limit;
	std::vector<uint16_t> nodeIdxes;;
};

// ���[�V�����֘A
struct KeyFrame {
	unsigned int frameNo;
	DirectX::XMVECTOR quaternion;
	DirectX::XMFLOAT3 offset;
	DirectX::XMFLOAT2 p1, p2;

	KeyFrame(unsigned int fno,
		const DirectX::XMVECTOR& q,
		const DirectX::XMFLOAT3& ofst,
		const DirectX::XMFLOAT2& ip1,
		const DirectX::XMFLOAT2& ip2) :
		frameNo(fno),
		quaternion(q),
		offset(ofst),
		p1(ip1),
		p2(ip2)
	{}
};

//IK�I���I�t�f�[�^
struct VMDIKEnable
{
	uint32_t frameNo;
	std::unordered_map<std::string, bool> ikEnableTable;
};

// ���f�������܂Ƃ߂��\����
struct Model
{
	// �S�̂̃C���f�b�N�X��
	unsigned int indices_num;
	// ���_�z��
	std::vector<DirectX::XMFLOAT3> vertices;

	// �}�e���A���֘A
	std::vector<Material> materials;
	ComPtr<ID3D12DescriptorHeap> material_heap = nullptr;
	ComPtr<ID3D12Resource> material_buffer = nullptr;
	std::vector<ComPtr<ID3D12Resource>> texture_resources;
	std::vector<ComPtr<ID3D12Resource>> sph_resources;
	std::vector<ComPtr<ID3D12Resource>> spa_resources;
	std::vector<ComPtr<ID3D12Resource>> toon_resources;

	// ���_�֘A
	ComPtr<ID3D12Resource> vertex_buffer = nullptr;
	ComPtr<ID3D12Resource> index_buffer = nullptr;

	// �r���[
	D3D12_VERTEX_BUFFER_VIEW vb_view = {};
	D3D12_INDEX_BUFFER_VIEW ib_view = {};

	// �{�[�����
	std::unordered_map<std::string, BoneNode> bone_node_table;
	std::vector<std::string> bone_name_array;
	std::vector<BoneNode*> bone_node_address_array;
	std::vector<PMDIK> ik_data;
	std::vector<uint32_t> knee_idxes;
};