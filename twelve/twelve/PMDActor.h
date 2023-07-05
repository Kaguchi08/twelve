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
	void Draw();
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
		std::string texPath;
		int toonIdx;
		bool edgeFlg;
	};

	// 全体をまとめたデータ
	struct Material
	{
		unsigned int indicesNum;
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
	ComPtr<ID3D12Resource> mMaterialBuffer = nullptr;
	std::vector<ComPtr<ID3D12Resource>> mTextureResources;
	std::vector<ComPtr<ID3D12Resource>> mSphResources;
	std::vector<ComPtr<ID3D12Resource>> mSpaResources;
	std::vector<ComPtr<ID3D12Resource>> mToonResources;

	// ボーン関連
	std::vector<DirectX::XMMATRIX> mBoneMatrices;

	struct BoneNode
	{
		uint32_t boneIdx;
		uint32_t boneType;
		uint32_t parentBone;
		uint32_t ikParentBone;
		DirectX::XMFLOAT3 startPos;
		std::vector<BoneNode*> children;
	};

	std::map<std::string, BoneNode> mBoneNodeTable;
	std::vector<std::string> mBoneNameArray;
	std::vector<BoneNode*> mBoneNodeAddressArray;


	struct PMDIK
	{
		uint16_t boneIdx;
		uint16_t targetBoneIdx;
		uint16_t iterations;
		float limit;
		std::vector<uint16_t> nodeIdxes;;
	};

	std::vector<PMDIK> mIKData;

	// マテリアルバッファの作成
	HRESULT CreateMaterialData();

	ComPtr<ID3D12DescriptorHeap> mMaterialHeap = nullptr;

	// マテリアルとテクスチャのビューを作成
	HRESULT CreateMaterialAndTextureView();
	// 座標変換用のビューを作成
	HRESULT CreateTransformView();

	// PMDファイルのロード
	HRESULT LoadPMDFile(const char* filepath);

	void RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat);

	float mAngle;

	// モーション関連
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

	std::unordered_map<std::string, std::vector<KeyFrame>> key_frames_;
	unsigned int duration_ = 0;

	DWORD mStartTime;
	std::vector<uint32_t> mKneeIdxes;

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

	void IKSolve(int frameNo);

	//IKオンオフデータ
	struct VMDIKEnable 
	{
		uint32_t frameNo;
		std::unordered_map<std::string, bool> ikEnableTable;
	};
	std::vector<VMDIKEnable> mIKEnableData;

	float GetYFromXOnBezier(float x, const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b, uint8_t n = 12);
};