#include "ResMesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <codecvt>
#include <cassert>

#include "Logger.h"

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

	std::wstring Convert(const aiString& path)
	{
		wchar_t temp[256] = {};
		size_t  size;
		mbstowcs_s(&size, temp, path.C_Str(), 256);
		return std::wstring(temp);
	}

	class MeshLoader
	{
	public:
		MeshLoader();
		~MeshLoader();

		bool Load(
			const wchar_t* fileName,
			std::vector<ResMesh>& meshes,
			std::vector<ResMaterial>& materials);

	private:
		const aiScene* m_pScene = nullptr;

		void ParseMesh(ResMesh& dstMesh, const aiMesh* pSrcMesh);
		void ParseMaterial(ResMaterial& dstMaterial, const aiMaterial* pSrcMaterial);
	};

	MeshLoader::MeshLoader()
		: m_pScene(nullptr)
	{
	}

	MeshLoader::~MeshLoader()
	{
	}

	bool MeshLoader::Load(const wchar_t* fileName, std::vector<ResMesh>& meshes, std::vector<ResMaterial>& materials)
	{
		if (fileName == nullptr)
		{
			ELOG("Error : Invalid Argument.");
			return false;
		}

		// wchar_t から char型(UTF-8)に変換する
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
		m_pScene = importer.ReadFile(path, flag);

		// チェック
		if (m_pScene == nullptr)
		{
			ELOG("Error : Assimp::Importer::ReadFile() Failed.");
			return false;
		}

		// メッシュのメモリを確保
		meshes.clear();
		meshes.resize(m_pScene->mNumMeshes);

		// メッシュデータを変換
		for (size_t i = 0; i < meshes.size(); ++i)
		{
			ParseMesh(meshes[i], m_pScene->mMeshes[i]);
		}

		// マテリアルのメモリを確保
		materials.clear();
		materials.resize(m_pScene->mNumMaterials);

		// マテリアルデータを変換
		for (size_t i = 0; i < materials.size(); ++i)
		{
			ParseMaterial(materials[i], m_pScene->mMaterials[i]);
		}

		// クリア
		importer.FreeScene();
		m_pScene = nullptr;

		return true;
	}

	void MeshLoader::ParseMesh(ResMesh& dstMesh, const aiMesh* pSrcMesh)
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

	void MeshLoader::ParseMaterial(ResMaterial& dstMaterial, const aiMaterial* pSrcMaterial)
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
				dstMaterial.Specular = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
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

		// 透過成分
		{
			auto alpha = 1.0f;

			if (pSrcMaterial->Get(AI_MATKEY_OPACITY, alpha) == AI_SUCCESS)
			{
				dstMaterial.Alpha = alpha;
			}
			else
			{
				dstMaterial.Alpha = 1.0f;
			}
		}

		// ディフーズマップ
		{
			aiString path;

			if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path) == AI_SUCCESS)
			{
				dstMaterial.DiffuseMap = Convert(path);
			}
			else
			{
				dstMaterial.DiffuseMap.clear();
			}
		}

		// スペキュラーマップ
		{
			aiString path;

			if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_SPECULAR(0), path) == AI_SUCCESS)
			{
				dstMaterial.SpecularMap = Convert(path);
			}
			else
			{
				dstMaterial.SpecularMap.clear();
			}
		}

		// シャイネスマップ
		{
			aiString path;

			if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_SHININESS(0), path) == AI_SUCCESS)
			{
				dstMaterial.ShininessMap = Convert(path);
			}
			else
			{
				dstMaterial.ShininessMap.clear();
			}
		}

		// 法線マップ
		{
			aiString path;

			if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_NORMALS(0), path) == AI_SUCCESS)
			{
				dstMaterial.NormalMap = Convert(path);
			}
			else
			{
				if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_HEIGHT(0), path) == AI_SUCCESS)
				{
					dstMaterial.NormalMap = Convert(path);
				}
				else
				{
					dstMaterial.NormalMap.clear();
				}
			}
		}

		// ベースカラー
		{
			aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);

			if (pSrcMaterial->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS)
			{
				dstMaterial.BaseColor = DirectX::XMFLOAT4(color.r, color.g, color.b, color.a);
			}
			else
			{
				dstMaterial.BaseColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		// 金属度
		{
			auto metallic = 0.0f;

			if (pSrcMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
			{
				dstMaterial.Metallic = metallic;
			}
			else
			{
				dstMaterial.Metallic = 0.0f;
			}
		}

		// 粗さ
		{
			auto roughness = 0.0f;

			if (pSrcMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
			{
				dstMaterial.Roughness = roughness;
			}
			else
			{
				dstMaterial.Roughness = 0.0f;
			}
		}

		// ベースカラーマップ
		{
			aiString path;

			if (pSrcMaterial->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &path) == AI_SUCCESS)
			{
				dstMaterial.BaseColorMap = Convert(path);
			}
			else
			{
				dstMaterial.BaseColorMap.clear();
			}
		}

		// 金属度・粗さマップ
		{
			aiString path;

			if (pSrcMaterial->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &path) == AI_SUCCESS)
			{
				dstMaterial.MetallicRoughnessMap = Convert(path);
			}
			else
			{
				dstMaterial.MetallicRoughnessMap.clear();
			}
		}


		{
			aiString path;

			if (pSrcMaterial->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &path) == AI_SUCCESS)
			{
				dstMaterial.MetallicMap = Convert(path);
			}
			else
			{
				dstMaterial.MetallicMap.clear();
			}
		}

		{
			aiString path;

			if (pSrcMaterial->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &path) == AI_SUCCESS)
			{
				dstMaterial.RoughnessMap = Convert(path);
			}
			else
			{
				dstMaterial.RoughnessMap.clear();
			}
		}
	}
}


const D3D12_INPUT_ELEMENT_DESC MeshVertex::InputElements[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};
const D3D12_INPUT_LAYOUT_DESC MeshVertex::InputLayout = {
	MeshVertex::InputElements,
	MeshVertex::InputElementCount
};
static_assert(sizeof(MeshVertex) == 44, "Vertex struct/layout mismatch");


bool LoadMesh(const wchar_t* fileName, std::vector<ResMesh>& meshes, std::vector<ResMaterial>& materials)
{
	MeshLoader loader;
	return loader.Load(fileName, meshes, materials);
}
