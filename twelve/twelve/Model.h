#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using Microsoft::WRL::ComPtr;

// �\����

enum ModelType {
    PMD,
    FBX,
};

// �V�F�[�_�[���ɓ�������}�e���A���f�[�^
struct MaterialForHlsl {
    DirectX::XMFLOAT3 diffuse;
    float alpha;  // �f�B�t���[�Y�̃A���t�@�l
    DirectX::XMFLOAT3 specular;
    float specularity;
    DirectX::XMFLOAT3 ambient;
};

// ���̑��̃}�e���A���f�[�^
struct AdditionalMaterial {
    std::string tex_path;
    int toon_idx;
    bool edge_flg;
};

// �S�̂��܂Ƃ߂��f�[�^
struct Material {
    unsigned int indices_num;
    MaterialForHlsl hlsl;
    AdditionalMaterial additional;
};

struct BoneNode {
    uint32_t bone_idx;
    uint32_t bone_type;
    uint32_t parent_bone;
    uint32_t ik_parent_bone;
    DirectX::XMFLOAT3 start_pos;
    std::vector<BoneNode*> children;
};

struct PMDIK {
    uint16_t bone_idx;
    uint16_t target_bone_idx;
    uint16_t iterations;
    float limit;
    std::vector<uint16_t> node_idxes;
    ;
};

// ���[�V�����֘A
struct KeyFrame {
    unsigned int frame_no;
    DirectX::XMVECTOR quaternion;
    DirectX::XMFLOAT3 offset;
    DirectX::XMFLOAT2 p1, p2;

    KeyFrame(unsigned int fno,
             const DirectX::XMVECTOR& q,
             const DirectX::XMFLOAT3& ofst,
             const DirectX::XMFLOAT2& ip1,
             const DirectX::XMFLOAT2& ip2) : frame_no(fno),
                                             quaternion(q),
                                             offset(ofst),
                                             p1(ip1),
                                             p2(ip2) {
    }
};

// IK�I���I�t�f�[�^
struct VMDIKEnable {
    uint32_t frame_no;
    std::unordered_map<std::string, bool> ik_enable_table;
};

// FBX���f���̒��_�f�[�^
struct FBXVertex {
    DirectX::XMFLOAT3 pos;       // x, y, z // ���W
    DirectX::XMFLOAT3 normal;    // nx, ny, nz // �@���x�N�g��
    DirectX::XMFLOAT2 uv;        // u, v // UV���W
    DirectX::XMFLOAT3 tangent;   // tx, ty, tz // �ڃx�N�g��
    DirectX::XMFLOAT3 binormal;  // bx, by, bz // �]�@���x�N�g��
};

// PMD���f�������܂Ƃ߂��\����
struct PMDModel {
    // �S�̂̃C���f�b�N�X��
    unsigned int indices_num;
    // ���_�z��
    std::vector<DirectX::XMFLOAT3> vertices;

    // �}�e���A���֘A
    std::vector<Material> material_table;
    ComPtr<ID3D12Resource> material_const_buffer = nullptr;
    ComPtr<ID3D12DescriptorHeap> material_cbv_heap = nullptr;
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

struct FBXMeshData {
    ComPtr<ID3D12Resource> vertex_buffer;
    ComPtr<ID3D12Resource> index_buffer;
    D3D12_VERTEX_BUFFER_VIEW vb_view;
    D3D12_INDEX_BUFFER_VIEW ib_view;
    std::vector<FBXVertex> vertices;
    std::vector<unsigned int> indices;
    std::string material_name;
};

struct FBXMaterial {
    float ambient[4];
    float diffuse[4];
    float specular[4];
};

// FBX���f�������܂Ƃ߂��\����
struct FBXModel {
    // �S�̂̃C���f�b�N�X��
    unsigned int indices_num;

    // ���b�V���f�[�^
    std::vector<FBXMeshData> mesh_data;

    // �}�e���A���֘A
    std::unordered_map<std::string, FBXMaterial> material_table;  // �}�e���A�����ƃ}�e���A���f�[�^�̑Ή��\
    std::unordered_map<std::string, ComPtr<ID3D12Resource>> material_const_buffer_table;
    std::unordered_map<std::string, ComPtr<ID3D12DescriptorHeap>> material_cbv_heap_table;

    // �e�N�X�`��
    std::unordered_map<std::string, ComPtr<ID3D12Resource>> texture_resource_table;        // �}�e���A�����ƃe�N�X�`�����\�[�X�̑Ή��\
    std::unordered_map<std::string, ComPtr<ID3D12DescriptorHeap>> texture_srv_heap_table;  // �}�e���A�����ƃe�N�X�`��SRV�q�[�v�̑Ή��\
};

struct VMDAnimation {
    std::unordered_map<std::string, std::vector<KeyFrame>> motion_table;
    std::vector<VMDIKEnable> ik_enable_table;
    unsigned int duration;
};

struct Animation {
    std::shared_ptr<VMDAnimation> vmd_anim;
    bool is_loop;
};