#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <codecvt>
#include <cassert>

namespace
{
	std::string ToUTF8(const std::wstring& value)
	{
		auto length = WideCharToMultiByte(
			CP_UTF8, 0U, value.data(), -1, nullptr, 0, nullptr, nullptr);
		auto buffer = new char[length];

		WideCharToMultiByte(
			CP_UTF8, 0U, value.data(), -1, buffer, length, nullptr, nullptr);

		std::string result(buffer);
		delete[] buffer;
		buffer = nullptr;

		return result;
	}

	class MeshLoader
	{
	public:
		MeshLoader();
		~MeshLoader();

		bool Load(
			const wchar_t* fileName,
			std::vector<Mesh>& meshes,
			std::vector<Material>& materials);

	private:
		void ParseMesh(Mesh& dstMesh, const aiMesh* pSrcMesh);
		void ParseMaterial(Material& dstMaterial, const aiMaterial* pSrcMaterial);
	};

	MeshLoader::MeshLoader()
	{
	}

	MeshLoader::~MeshLoader()
	{
	}

	bool MeshLoader::Load(const wchar_t* fileName, std::vector<Mesh>& meshes, std::vector<Material>& materials)
	{
		if (fileName == nullptr)
		{
			return false;
		}

		// wchar_t から char型(UTF-8)に変換します
		auto path = ToUTF8(fileName);

		Assimp::Importer importer;
		int flag = 0;
		flag |= aiProcess_Triangulate;
		flag |= aiProcess_PreTransformVertices;
		flag |= aiProcess_CalcTangentSpace;
		flag |= aiProcess_GenSmoothNormals;
		flag |= aiProcess_GenUVCoords;
		flag |= aiProcess_RemoveRedundantMaterials;
		flag |= aiProcess_OptimizeMeshes;

		// ファイルを読み込み
		auto pScene = importer.ReadFile(path, flag);

		// チェック
		if (pScene == nullptr)
		{
			return false;
		}

		// メッシュのメモリを確保
		meshes.clear();
		meshes.resize(pScene->mNumMeshes);

		// メッシュデータを変換
		for (size_t i = 0; i < meshes.size(); ++i)
		{
			ParseMesh(meshes[i], pScene->mMeshes[i]);
		}

		// マテリアルのメモリを確保
		materials.clear();
		materials.resize(pScene->mNumMaterials);

		// マテリアルデータを変換
		for (size_t i = 0; i < materials.size(); ++i)
		{
			ParseMaterial(materials[i], pScene->mMaterials[i]);
		}

		// クリア
		pScene = nullptr;

		return true;
	}

	void MeshLoader::ParseMesh(Mesh& dstMesh, const aiMesh* pSrcMesh)
	{
		// マテリアル番号を設定
		dstMesh.MaterialId = pSrcMesh->mMaterialIndex;

		aiVector3D zero3D(0.0f, 0.0f, 0.0f);

		// 頂点データのメモリを確保
		dstMesh.Vertices.resize(pSrcMesh->mNumVertices);

		for (auto i = 0u; i < pSrcMesh->mNumVertices; ++i)
		{
			auto pPosition = &(pSrcMesh->mVertices[i]);
			auto pNormal = &(pSrcMesh->mNormals[i]);
			auto pTexCoord = (pSrcMesh->HasTextureCoords(0)) ? &(pSrcMesh->mTextureCoords[0][i]) : &zero3D;
			auto pTangent = (pSrcMesh->HasTangentsAndBitangents()) ? &(pSrcMesh->mTangents[i]) : &zero3D;

			dstMesh.Vertices[i] = MeshVertex(
				DirectX::XMFLOAT3(pPosition->x, pPosition->y, pPosition->z),
				DirectX::XMFLOAT3(pNormal->x, pNormal->y, pNormal->z),
				DirectX::XMFLOAT2(pTexCoord->x, pTexCoord->y),
				DirectX::XMFLOAT3(pTangent->x, pTangent->y, pTangent->z));
		}

		// インデックスデータのメモリを確保
		dstMesh.Indices.resize(pSrcMesh->mNumFaces * 3);

		for (auto i = 0u; i < pSrcMesh->mNumFaces; ++i)
		{
			const auto& face = pSrcMesh->mFaces[i];

			assert(face.mNumIndices == 3); // 三角形化しているので必ず3になっている

			dstMesh.Indices[i * 3 + 0] = face.mIndices[0];
			dstMesh.Indices[i * 3 + 1] = face.mIndices[1];
			dstMesh.Indices[i * 3 + 2] = face.mIndices[2];
		}
	}

	void MeshLoader::ParseMaterial(Material& dstMaterial, const aiMaterial* pSrcMaterial)
	{
		// 拡散反射成分
		{
			aiColor3D color(0.0f, 0.0f, 0.0f);

			if (pSrcMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
			{
				dstMaterial.Diffuse = DirectX::XMFLOAT3(color.r, color.g, color.b);
			}
			else
			{
				dstMaterial.Diffuse = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);
			}
		}

		// 鏡面反射成分
		{
			aiColor3D color(0.0f, 0.0f, 0.0f);

			if (pSrcMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
			{
				dstMaterial.Specular = DirectX::XMFLOAT3(color.r, color.g, color.b);
			}
			else
			{
				dstMaterial.Specular = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);
			}
		}

		// 鏡面反射強度
		{
			auto shininess = 0.0f;

			if (pSrcMaterial->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
			{
				dstMaterial.Shininess = shininess;
			}
			else
			{
				dstMaterial.Shininess = 0.0f;
			}
		}

		// 透過成分は？

		// ディフーズマップ
		{
			aiString path;

			if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path) == AI_SUCCESS)
			{
				dstMaterial.DiffuseMap = std::string(path.C_Str());
			}
			else
			{
				dstMaterial.DiffuseMap.clear();
			}
		}
	}
}

#define FMT_FLOAT3      DXGI_FORMAT_R32G32B32_FLOAT
#define FMT_FLOAT2      DXGI_FORMAT_R32G32_FLOAT
#define APPEND          D3D12_APPEND_ALIGNED_ELEMENT
#define IL_VERTEX       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA

const D3D12_INPUT_ELEMENT_DESC MeshVertex::InputElements[] = {
	{ "POSITION", 0, FMT_FLOAT3, 0, APPEND, IL_VERTEX, 0 },
	{ "NORMAL",   0, FMT_FLOAT3, 0, APPEND, IL_VERTEX, 0 },
	{ "TEXCOORD", 0, FMT_FLOAT2, 0, APPEND, IL_VERTEX, 0 },
	{ "TANGENT",  0, FMT_FLOAT3, 0, APPEND, IL_VERTEX, 0 }
};
const D3D12_INPUT_LAYOUT_DESC MeshVertex::InputLayout = {
	MeshVertex::InputElements,
	MeshVertex::InputElementCount
};
static_assert(sizeof(MeshVertex) == 44, "Vertex struct/layout mismatch");

#undef FMT_FLOAT3
#undef FMT_FLOAT2
#undef APPEND
#undef IL_VERTEX


bool LoadMesh(const wchar_t* fileName, std::vector<Mesh>& meshes, std::vector<Material>& materials)
{
	MeshLoader loader;
	return loader.Load(fileName, meshes, materials);
}
