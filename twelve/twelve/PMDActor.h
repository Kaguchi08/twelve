#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

class Dx12Wrapper;
class PMDRenderer;

class PMDActor
{
	friend PMDRenderer;

public:
	PMDActor(const char* filepath, PMDRenderer& renderer);
	~PMDActor();

	void Update();
	void DrawToBackBuffer();
	void PlayAnimation();

	void LoadVMDFile(const char* filepath);



private:
	Dx12Wrapper& dx12_;
	PMDRenderer& renderer_;

	// 頂点関連
	ComPtr<ID3D12Resource> vertex_buffer_ = nullptr;
	ComPtr<ID3D12Resource> index_buffer_ = nullptr;
	ComPtr<ID3D12Resource> mTransformMat = nullptr;

	// ビュー
	D3D12_VERTEX_BUFFER_VIEW vb_view_ = {};
	D3D12_INDEX_BUFFER_VIEW ib_view_ = {};
	ComPtr<ID3D12DescriptorHeap> mTransformHeap = nullptr;

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

	struct Transform
	{
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};

	Transform mTransform;
	DirectX::XMMATRIX* mMappedMatrices = nullptr;
	ComPtr<ID3D12Resource> mTransformBuffer = nullptr;

	// マテリアル関連
	std::vector<Material> mMaterials;
	ComPtr<ID3D12Resource> material_const_buffer = nullptr;
	std::vector<ComPtr<ID3D12Resource>> texture_resources;
	std::vector<ComPtr<ID3D12Resource>> sph_resources;
	std::vector<ComPtr<ID3D12Resource>> spa_resources;
	std::vector<ComPtr<ID3D12Resource>> toon_resources;

	// ボーン関連
	std::vector<DirectX::XMMATRIX> mBoneMatrices;

	struct BoneNode
	{
		uint32_t bone_idx;
		uint32_t bone_type;
		uint32_t parent_bone;
		uint32_t ik_parent_bone;
		DirectX::XMFLOAT3 start_pos;
		std::vector<BoneNode*> children;
	};

	std::map<std::string, BoneNode> bone_node_table;
	std::vector<std::string> bone_name_array;
	std::vector<BoneNode*> bone_node_address_array;


	struct PMDIK
	{
		uint16_t bone_idx;
		uint16_t target_bone_idx;
		uint16_t iterations;
		float limit;
		std::vector<uint16_t> node_idxes;;
	};

	std::vector<PMDIK> ik_data;

	// マテリアルバッファの作成
	HRESULT CreateMaterialData();

	ComPtr<ID3D12DescriptorHeap> material_cbv_heap = nullptr;

	// マテリアルとテクスチャのビューを作成
	HRESULT CreateMaterialAndTextureView();
	// 座標変換用のビューを作成
	HRESULT CreateTransformView();

	// PMDファイルのロード
	HRESULT LoadPMDFile(const char* filepath);

	void RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat);

	float mAngle;

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

	std::unordered_map<std::string, std::vector<KeyFrame>> key_frames_;
	unsigned int duration_ = 0;

	DWORD mStartTime;
	std::vector<uint32_t> knee_idxes;

	void MotionUpdate();

	// IK関連

	/// <summary>
	/// CCD-IKによりボーン方向を解決
	/// </summary>
	/// <param name="ik"></param>
	void SolveCCDIK(const PMDIK& ik);

	/// <summary>
	/// 余弦定理IKによりボーン方向を解決
	/// </summary>
	/// <param name="ik"></param>
	void SolveCosineIK(const PMDIK& ik);

	/// <summary>
	/// LookAt行列によりボーン方向を解決
	/// </summary>
	/// <param name="ik"></param>
	void SolveLookAt(const PMDIK& ik);

	void IKSolve(int frame_no);

	//IKオンオフデータ
	struct VMDIKEnable
	{
		uint32_t frame_no;
		std::unordered_map<std::string, bool> ik_enable_table;
	};
	std::vector<VMDIKEnable> mIKEnableData;

	float GetYFromXOnBezier(float x, const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b, uint8_t n = 12);
};