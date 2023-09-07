#include "ModelLoader.h"
#include "Renderer.h"
#include "Dx12Wrapper.h"
#include <string>
#include <assert.h>
#include <DirectXMath.h>
#include <d3dx12.h>
#include <algorithm>

ModelLoader::ModelLoader(Renderer& renderer) :
	dx12_(renderer.GetDx()),
	renderer_(renderer)
{
}

ModelLoader::~ModelLoader()
{
}

bool ModelLoader::LoadPMDModel(const char* file_name, PMDModel* model)
{
	FILE* fp;
	std::string strModelPath = file_name;

	fopen_s(&fp, strModelPath.c_str(), "rb");

	if (fp == nullptr)
	{
		assert(0);
		fclose(fp);
		return false;
	}

	// ヘッダーの読み込み
	// PMD ヘッダー構造体
	struct PMDHeader
	{
		float version; // 00 00 80 3F == 1.00
		char modelName[20]; // モデル名
		char comment[256]; // コメント
	};

	char signature[3]; // "PMD"
	PMDHeader pmd_header = {};

	fread(signature, sizeof(char), 3, fp);
	fread(&pmd_header, sizeof(PMDHeader), 1, fp);

	// 頂点数
	unsigned int vert_num;
	fread(&vert_num, sizeof(unsigned int), 1, fp);

	// 1バイトパッキング（読み込みは1度のみ）
#pragma pack(1)
	// PMDマテリアル構造体
	struct PMDMaterial
	{
		DirectX::XMFLOAT3 diffuse; // ディフューズ色
		float alpha; // ディフューズのアルファ値
		float specularity; // スペキュラの強さ(乗算値)
		DirectX::XMFLOAT3 specular; // スペキュラ色
		DirectX::XMFLOAT3 ambient; // アンビエント色
		unsigned char toon_idx; // トゥーン番号(0～9)
		unsigned char edge_flg; // 輪郭線フラグ
		unsigned int indices_num; // 面頂点数
		char tex_path[20]; // テクスチャパス
	};

	struct PMDVertex
	{
		float pos[3]; // x, y, z // 座標
		float normal_vec[3]; // nx, ny, nz // 法線ベクトル
		float uv[2]; // u, v // UV座標 // MMDは頂点UV
		WORD bone_num[2]; // ボーン番号1、番号2 // モデル変形(頂点移動)時に影響
		BYTE bone_weight; // ボーン1に与える影響度 // min:0 max:100 // ボーン2への影響度は、(100 - bone_weight)
		BYTE edge_flag; // 0:通常、1:エッジ無効 // エッジ(輪郭)が有効の場合
	};
#pragma pack()

	constexpr unsigned int vertex_size = sizeof(PMDVertex);
	std::vector<PMDVertex> vertices(vert_num);

	// 頂点データの読み込み
	fread(vertices.data(), vertex_size, vert_num, fp);

	// 面頂点数の合計
	unsigned int indices_num;
	fread(&indices_num, sizeof(unsigned int), 1, fp);
	model->indices_num = indices_num;

	// 頂点バッファの作成
	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(vertices[0]));

	auto result = dx12_.GetDevice()->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(model->vertex_buffer.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		fclose(fp);
		return false;
	}

	unsigned char* vert_map = nullptr;
	result = model->vertex_buffer->Map(0, nullptr, (void**)&vert_map);

	if (FAILED(result))
	{
		assert(0);
		fclose(fp);
		return false;
	}

	model->vertices.resize(vert_num);

	DWORD i = 0;
	for (auto& v : vertices)
	{
		model->vertices[i] = DirectX::XMFLOAT3(v.pos[0], v.pos[1], v.pos[2]);
		*((PMDVertex*)vert_map) = v;
		vert_map += vertex_size;
		i++;
	}
	model->vertex_buffer->Unmap(0, nullptr);

	model->vb_view.BufferLocation = model->vertex_buffer->GetGPUVirtualAddress();
	model->vb_view.SizeInBytes = vertices.size() * vertex_size;
	model->vb_view.StrideInBytes = vertex_size;

	// インデックスデータの読み込み
	std::vector<unsigned short> indices(indices_num);
	fread(indices.data(), sizeof(unsigned short), indices_num, fp);

	// インデックスバッファの作成
	res_desc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));

	result = dx12_.GetDevice()->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(model->index_buffer.ReleaseAndGetAddressOf())
	);

	unsigned short* index_map = nullptr;
	result = model->index_buffer->Map(0, nullptr, (void**)&index_map);

	if (FAILED(result))
	{
		assert(0);
		fclose(fp);
		return false;
	}

	std::copy(indices.begin(), indices.end(), index_map);
	model->index_buffer->Unmap(0, nullptr);

	model->ib_view.BufferLocation = model->index_buffer->GetGPUVirtualAddress();
	model->ib_view.Format = DXGI_FORMAT_R16_UINT;
	model->ib_view.SizeInBytes = indices.size() * sizeof(indices[0]);

	// マテリアル数
	unsigned int material_num;
	fread(&material_num, sizeof(unsigned int), 1, fp);

	// vector のサイズ
	model->material_table.resize(material_num);
	model->texture_resources.resize(material_num);
	model->sph_resources.resize(material_num);
	model->spa_resources.resize(material_num);
	model->toon_resources.resize(material_num);

	// マテリアルの読み込み
	std::vector<PMDMaterial> material_table(material_num);
	fread(material_table.data(), sizeof(PMDMaterial), material_num, fp);

	// コピー
	for (int i = 0; i < material_table.size(); ++i)
	{
		auto& material = model->material_table[i];
		material.indices_num = material_table[i].indices_num;
		material.hlsl.diffuse = material_table[i].diffuse;
		material.hlsl.alpha = material_table[i].alpha;
		material.hlsl.specular = material_table[i].specular;
		material.hlsl.specularity = material_table[i].specularity;
		material.hlsl.ambient = material_table[i].ambient;
		material.additional.toon_idx = material_table[i].toon_idx;

		// toon
		//std::string toonFilePath = "../Assets/Model/Kafka/";
		std::string toonFilePath = "../toon/";
		char toonFileName[32];
		//sprintf_s(toonFileName, 32, "toon%d.png", pmdMaterials[i].toonIdx + 1);
		sprintf_s(toonFileName, 32, "toon%02d.bmp", material_table[i].toon_idx + 1);
		toonFilePath += toonFileName;

		model->toon_resources[i] = dx12_.GetResourceManager()->GetTextureFromPath(toonFilePath.c_str());

		if (strlen(material_table[i].tex_path) == 0)
		{
			model->texture_resources[i] = nullptr;
			continue;
		}

		std::string texFileName = material_table[i].tex_path;
		std::string sphFileName = "";
		std::string spaFileName = "";

		if (std::count(texFileName.begin(), texFileName.end(), '*') > 0)
		{
			auto namePair = SplitFileName(texFileName);

			if (GetExtension(namePair.first) == "sph")
			{
				texFileName = namePair.second;
				sphFileName = namePair.first;
			}
			else if (GetExtension(namePair.first) == "spa")
			{
				texFileName = namePair.second;
				spaFileName = namePair.first;
			}
			else
			{
				texFileName = namePair.first;
				if (GetExtension(namePair.second) == "sph")
				{
					sphFileName = namePair.second;
				}
				else if (GetExtension(namePair.second) == "spa")
				{
					spaFileName = namePair.second;
				}
			}
		}
		else
		{
			if (GetExtension(material_table[i].tex_path) == "sph")
			{
				texFileName = "";
				sphFileName = material_table[i].tex_path;
			}
			else if (GetExtension(material_table[i].tex_path) == "spa")
			{
				texFileName = "";
				spaFileName = material_table[i].tex_path;
			}
			else
			{
				texFileName = material_table[i].tex_path;
			}
		}

		if (texFileName != "")
		{
			auto texFilePath = GetTexturePathFromModelAndTexPath(strModelPath, texFileName.c_str());
			model->texture_resources[i] = dx12_.GetResourceManager()->GetTextureFromPath(texFilePath.c_str());
		}

		if (sphFileName != "")
		{
			auto sphFilePath = GetTexturePathFromModelAndTexPath(strModelPath, sphFileName.c_str());
			model->sph_resources[i] = dx12_.GetResourceManager()->GetTextureFromPath(sphFilePath.c_str());
		}

		if (spaFileName != "")
		{
			auto spaFilePaht = GetTexturePathFromModelAndTexPath(strModelPath, spaFileName.c_str());
			model->spa_resources[i] = dx12_.GetResourceManager()->GetTextureFromPath(spaFilePaht.c_str());
		}
	}

	// マテリアルバッファの作成
	result = CreatePMDMaterialData(model);

	if (FAILED(result))
	{
		assert(0);
		fclose(fp);
		return false;
	}

	// マテリアルのビューの作成
	result = CreatePMDMaterialAndView(model);

	if (FAILED(result))
	{
		assert(0);
		fclose(fp);
		return false;
	}

	// ボーンの読み込み
	unsigned short bone_num;
	fread(&bone_num, sizeof(unsigned short), 1, fp);

#pragma pack(1)
	// 読み取り用bone構造体
	struct PMDBone
	{
		char bone_name[20]; // ボーン名
		unsigned short parent_no; // 親ボーン番号
		unsigned short next_no; // 先端ボーン番号
		unsigned char type; // ボーンの種類
		unsigned short ik_bone_no; // IKボーン番号
		DirectX::XMFLOAT3 pos; // ボーンの位置
	};
#pragma pack()

	std::vector<PMDBone> bones(bone_num);
	fread(bones.data(), sizeof(PMDBone), bone_num, fp);

	uint16_t ik_num = 0;
	fread(&ik_num, sizeof(ik_num), 1, fp);

	model->ik_data.resize(ik_num);

	for (auto& ik : model->ik_data)
	{
		fread(&ik.bone_idx, sizeof(ik.bone_idx), 1, fp);
		fread(&ik.target_bone_idx, sizeof(ik.target_bone_idx), 1, fp);
		uint8_t chainLen = 0;
		fread(&chainLen, sizeof(chainLen), 1, fp);
		ik.node_idxes.resize(chainLen);
		fread(&ik.iterations, sizeof(ik.iterations), 1, fp);
		fread(&ik.limit, sizeof(ik.limit), 1, fp);

		if (chainLen == 0)
		{
			continue;
		}

		fread(ik.node_idxes.data(), sizeof(ik.node_idxes[0]), chainLen, fp);
	}

	fclose(fp);

	model->bone_name_array.resize(bone_num);
	model->bone_node_address_array.resize(bone_num);
	model->knee_idxes.clear();

	// ボーンノードマップの作成
	for (int idx = 0; idx < bone_num; ++idx)
	{
		auto& bone = bones[idx];
		auto& node = model->bone_node_table[bone.bone_name];
		node.bone_idx = idx;
		node.start_pos = bone.pos;
		node.bone_type = bone.type;
		node.parent_bone = bone.parent_no;
		node.ik_parent_bone = bone.ik_bone_no;

		model->bone_name_array[idx] = bone.bone_name;
		model->bone_node_address_array[idx] = &node;

		std::string boneName = bone.bone_name;
		if (boneName.find("ひざ") != std::string::npos)
		{
			model->knee_idxes.emplace_back(idx);
		}
	}

	// ボーンの親子関係を構築
	for (auto& bone : bones)
	{
		if (bone.parent_no >= bones.size())
		{
			continue;
		}

		auto parent_name = model->bone_name_array[bone.parent_no];
		model->bone_node_table[parent_name].children.emplace_back(&model->bone_node_table[bone.bone_name]);
	}

	return true;
}

bool ModelLoader::LoadFBXModel(const char* file_name, FBXModel* model)
{
	// FBX Managerの作成
	FbxManager* manager = FbxManager::Create();
	if (manager == nullptr)
	{
		return false;
	}

	// FBX Importerの作成
	FbxImporter* importer = FbxImporter::Create(manager, "");
	if (importer == nullptr)
	{
		manager->Destroy();
		return false;
	}

	// FBX Sceneの作成
	FbxScene* scene = FbxScene::Create(manager, "");
	if (scene == nullptr)
	{
		importer->Destroy();
		manager->Destroy();
		return false;
	}

	// Fileを初期化
	if (importer->Initialize(file_name) == false)
	{
		importer->Destroy();
		scene->Destroy();
		manager->Destroy();
		return false;
	}
	// SeneにFileをImport
	if (importer->Import(scene) == false)
	{
		importer->Destroy();
		scene->Destroy();
		manager->Destroy();
		return false;
	}

	FbxGeometryConverter converter(manager);

	// マテリアル単位でメッシュを分割
	converter.SplitMeshesPerMaterial(scene, true);
	// ポリゴンを三角形に
	converter.Triangulate(scene, true);

	int material_num = scene->GetSrcObjectCount<FbxSurfaceMaterial>();

	for (int i = 0; i < material_num; i++)
	{
		LoadMaterial(scene->GetSrcObject<FbxSurfaceMaterial>(i), model);
	}

	int mesh_num = scene->GetSrcObjectCount<FbxMesh>();

	for (int i = 0; i < mesh_num; i++)
	{
		CreateFBXMesh(scene->GetSrcObject<FbxMesh>(i), model);
	}

	importer->Destroy();
	scene->Destroy();
	manager->Destroy();

	CreateFBXVertexBuffers(model);
	CreateFBXIndexBuffers(model);

	return true;
}

bool ModelLoader::LoadVMDFile(const char* file_name, VMDAnimation* anim)
{
	FILE* fp;
	fopen_s(&fp, file_name, "rb");
	if (fp == nullptr)
	{
		assert(0);
		fclose(fp);
		return false;
	}

	// ヘッダー
	fseek(fp, 50, SEEK_SET);

	// モーションデータ
	unsigned int motion_num = 0;
	fread(&motion_num, sizeof(motion_num), 1, fp);

	// VMDモーション構造体
	struct VMDMotion
	{
		char bone_name[15];
		unsigned int frame_no;
		DirectX::XMFLOAT3 location;
		DirectX::XMFLOAT4 quaternion;
		unsigned char bezier[64];
	};

	std::vector<VMDMotion> motions(motion_num);

	// モーションデータの読み込み
	for (auto& motion : motions)
	{
		fread(motion.bone_name, sizeof(motion.bone_name), 1, fp);
		fread(&motion.frame_no,
			  sizeof(motion.frame_no)
			  + sizeof(motion.location)
			  + sizeof(motion.quaternion)
			  + sizeof(motion.bezier),
			  1,
			  fp
		);
	}

	// 使用するモーションテーブルへ変換
	for (auto& motion : motions)
	{
		anim->motion_table[motion.bone_name].emplace_back(
			KeyFrame(motion.frame_no,
					 DirectX::XMLoadFloat4(&motion.quaternion),
					 motion.location,
					 DirectX::XMFLOAT2((float)motion.bezier[3] / 127.0f, (float)motion.bezier[7] / 127.0f),
					 DirectX::XMFLOAT2((float)motion.bezier[11] / 127.0f, (float)motion.bezier[15] / 127.0f))
		);

		anim->duration = std::max<unsigned int>(anim->duration, motion.frame_no);
	}

	// モーションデータのソート
	for (auto& motion : anim->motion_table)
	{
		std::sort(motion.second.begin(), motion.second.end(),
				  [](const KeyFrame& lval, const KeyFrame& rval)
				  {
					  return lval.frame_no < rval.frame_no;
				  }
		);
	}

#pragma pack(1)
	//表情データ(頂点モーフデータ)
	struct VMDMorph
	{
		char name[15];//名前(パディングしてしまう)
		uint32_t frame_no;//フレーム番号
		float weight;//ウェイト(0.0f～1.0f)
	};
#pragma pack()

	uint32_t morph_count = 0;
	fread(&morph_count, sizeof(morph_count), 1, fp);
	std::vector<VMDMorph> morphs(morph_count);
	fread(morphs.data(), sizeof(VMDMorph), morph_count, fp);

#pragma pack(1)
	//カメラ
	struct VMDCamera
	{
		uint32_t frame_no; // フレーム番号
		float distance; // 距離
		DirectX::XMFLOAT3 pos; // 座標
		DirectX::XMFLOAT3 euler_angle; // オイラー角
		uint8_t interpolation[24]; // 補完
		uint32_t fov; // 視界角
		uint8_t persFlg; // パースフラグON/OFF
	};
#pragma pack()

	uint32_t vmd_camera_count = 0;
	fread(&vmd_camera_count, sizeof(vmd_camera_count), 1, fp);
	std::vector<VMDCamera> cameraData(vmd_camera_count);
	fread(cameraData.data(), sizeof(VMDCamera), vmd_camera_count, fp);

	// ライト照明データ
	struct VMDLight
	{
		uint32_t frame_no; // フレーム番号
		DirectX::XMFLOAT3 rgb; //ライト色
		DirectX::XMFLOAT3 vec; //光線ベクトル(平行光線)
	};

	uint32_t vmd_light_count = 0;
	fread(&vmd_light_count, sizeof(vmd_light_count), 1, fp);
	std::vector<VMDLight> lights(vmd_light_count);
	fread(lights.data(), sizeof(VMDLight), vmd_light_count, fp);

#pragma pack(1)
	// セルフ影データ
	struct VMDSelfShadow
	{
		uint32_t frame_no; // フレーム番号
		uint8_t mode; //影モード(0:影なし、1:モード１、2:モード２)
		float distance; //距離
	};
#pragma pack()

	uint32_t self_shadow_count = 0;
	fread(&self_shadow_count, sizeof(self_shadow_count), 1, fp);
	std::vector<VMDSelfShadow> selfShadowData(self_shadow_count);
	fread(selfShadowData.data(), sizeof(VMDSelfShadow), self_shadow_count, fp);

	// IKオンオフ切り替わり数
	uint32_t ik_switch_count = 0;
	fread(&ik_switch_count, sizeof(ik_switch_count), 1, fp);

	anim->ik_enable_table.resize(ik_switch_count);

	for (auto& ik_enable : anim->ik_enable_table)
	{
		fread(&ik_enable.frame_no, sizeof(ik_enable.frame_no), 1, fp);
		// 可視フラグは使用しない
		uint8_t visible_flg = 0;
		fread(&visible_flg, sizeof(visible_flg), 1, fp);

		// 対象ボーン数読み込み
		uint32_t ik_bone_count = 0;
		fread(&ik_bone_count, sizeof(ik_bone_count), 1, fp);

		// ON/OFF情報を取得
		for (int i = 0; i < ik_bone_count; ++i)
		{
			char ik_bone_name[20];
			fread(ik_bone_name, _countof(ik_bone_name), 1, fp);
			uint8_t flg = 0;
			fread(&flg, sizeof(flg), 1, fp);
			ik_enable.ik_enable_table[ik_bone_name] = flg;
		}
	}

	fclose(fp);

	return true;
}

HRESULT ModelLoader::CreatePMDMaterialData(struct PMDModel* model)
{
	// マテリアルバッファの作成
	auto material_buffer_size = AligmentedValue(sizeof(MaterialForHlsl), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT); // 256の倍数にする

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(material_buffer_size * model->material_table.size());

	auto result = dx12_.GetDevice()->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(model->material_const_buffer.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// マテリアルバッファのマップ
	char* mapped_material = nullptr;

	result = model->material_const_buffer->Map(0, nullptr, (void**)&mapped_material);
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// マテリアルバッファへの書き込み
	for (auto& material : model->material_table)
	{
		*((MaterialForHlsl*)mapped_material) = material.hlsl;
		mapped_material += material_buffer_size;
	}

	// マテリアルバッファのアンマップ
	model->material_const_buffer->Unmap(0, nullptr);

	return S_OK;
}

HRESULT ModelLoader::CreatePMDMaterialAndView(struct PMDModel* model)
{
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	// マテリアルの数分 material, texture, sph, spa, toon
	heap_desc.NumDescriptors = model->material_table.size() * 5;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.NodeMask = 0;

	auto result = dx12_.GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(model->material_cbv_heap.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	auto material_buffer_size = AligmentedValue(sizeof(MaterialForHlsl), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT); // 256の倍数にする

	// View 作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = model->material_const_buffer->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = material_buffer_size;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(model->material_cbv_heap->GetCPUDescriptorHandleForHeapStart());
	auto inc_size = dx12_.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < model->material_table.size(); ++i)
	{
		dx12_.GetDevice()->CreateConstantBufferView(&cbv_desc, handle);
		handle.ptr += inc_size; // Offset()でもいける
		cbv_desc.BufferLocation += material_buffer_size;

		if (model->texture_resources[i] == nullptr)
		{
			srv_desc.Format = renderer_.white_texture_->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(renderer_.white_texture_.Get(), &srv_desc, handle);
		}
		else
		{
			srv_desc.Format = model->texture_resources[i]->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(model->texture_resources[i].Get(), &srv_desc, handle);
		}
		handle.ptr += inc_size;

		if (model->sph_resources[i] == nullptr)
		{
			srv_desc.Format = renderer_.white_texture_->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(renderer_.white_texture_.Get(), &srv_desc, handle);
		}
		else
		{
			srv_desc.Format = model->sph_resources[i]->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(model->sph_resources[i].Get(), &srv_desc, handle);
		}
		handle.ptr += inc_size;

		if (model->spa_resources[i] == nullptr)
		{
			srv_desc.Format = renderer_.black_texture_->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(renderer_.black_texture_.Get(), &srv_desc, handle);
		}
		else
		{
			srv_desc.Format = model->spa_resources[i]->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(model->spa_resources[i].Get(), &srv_desc, handle);
		}
		handle.ptr += inc_size;

		if (model->toon_resources[i] == nullptr)
		{
			srv_desc.Format = renderer_.grad_texture_->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(renderer_.grad_texture_.Get(), &srv_desc, handle);
		}
		else
		{
			srv_desc.Format = model->toon_resources[i]->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(model->toon_resources[i].Get(), &srv_desc, handle);
		}
		handle.ptr += inc_size;
	}

	return S_OK;
}

HRESULT ModelLoader::CreateFBXMaterialData(FBXModel* model)
{
	return S_OK;
}

HRESULT ModelLoader::CreateFBXMaterialAndView(FBXModel* model)
{
	return S_OK;
}

void ModelLoader::CollectFBXMeshNode(FbxNode* node, std::unordered_map<std::string, FbxNode*>& table)
{
	for (int i = 0; i < node->GetNodeAttributeCount(); i++)
	{
		FbxNodeAttribute* attr = node->GetNodeAttributeByIndex(i);

		// AttributeがMeshの場合はテーブルに登録
		if (attr->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			table[node->GetName()] = node;
			break;
		}
	}

	for (int i = 0; i < node->GetChildCount(); i++)
	{
		CollectFBXMeshNode(node->GetChild(i), table);
	}
}

bool ModelLoader::CreateFBXMesh(FbxMesh* mesh, struct FBXModel* model)
{
	FBXMeshData mesh_data;
	LoadIndices(mesh, mesh_data);
	LoadVertices(mesh, mesh_data);
	LoadNormals(mesh, mesh_data);
	SetMaterialName(mesh, mesh_data);

	model->mesh_data.push_back(mesh_data);

	return true;
}

bool ModelLoader::CreateFBXVertexBuffers(FBXModel* model)
{
	for (auto& mesh : model->mesh_data)
	{
		// 頂点バッファの作成
		auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(mesh.vertices.size() * sizeof(FBXVertex));

		auto result = dx12_.GetDevice()->CreateCommittedResource(
			&heap_prop,
			D3D12_HEAP_FLAG_NONE,
			&res_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(mesh.vertex_buffer.ReleaseAndGetAddressOf())
		);

		if (FAILED(result))
		{
			return false;
		}

		// 頂点バッファへの書き込み
		unsigned char* vertex_map = nullptr;
		result = mesh.vertex_buffer->Map(0, nullptr, (void**)(&vertex_map));

		if (FAILED(result))
		{
			return false;
		}

		DWORD i = 0;
		constexpr unsigned int vertex_size = sizeof(FBXVertex);
		for (auto& v : mesh.vertices)
		{
			//model->vertices[i] = DirectX::XMFLOAT3(v.pos[0], v.pos[1], v.pos[2]);
			*((FBXVertex*)vertex_map) = v;
			vertex_map += vertex_size;
			i++;
		}
		mesh.vertex_buffer->Unmap(0, nullptr);

		mesh.vb_view.BufferLocation = mesh.vertex_buffer->GetGPUVirtualAddress();
		mesh.vb_view.SizeInBytes = mesh.vertices.size() * vertex_size;
		mesh.vb_view.StrideInBytes = vertex_size;
	}
}

bool ModelLoader::CreateFBXIndexBuffers(FBXModel* model)
{
	for (auto& mesh : model->mesh_data)
	{
		// インデックスバッファの作成
		auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(mesh.indices.size() * sizeof(unsigned int));

		auto result = dx12_.GetDevice()->CreateCommittedResource(
			&heap_prop,
			D3D12_HEAP_FLAG_NONE,
			&res_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(mesh.index_buffer.ReleaseAndGetAddressOf())
		);

		if (FAILED(result))
		{
			return false;
		}

		// インデックスバッファへの書き込み
		unsigned short* index_map = nullptr;
		result = mesh.index_buffer->Map(0, nullptr, (void**)&index_map);

		if (FAILED(result))
		{
			return false;
		}

		std::copy(mesh.indices.begin(), mesh.indices.end(), index_map);
		mesh.index_buffer->Unmap(0, nullptr);

		mesh.ib_view.BufferLocation = mesh.index_buffer->GetGPUVirtualAddress();
		mesh.ib_view.Format = DXGI_FORMAT_R16_UINT;
		mesh.ib_view.SizeInBytes = mesh.indices.size() * sizeof(unsigned int);
	}
}

void ModelLoader::LoadVertices(FbxMesh* mesh, FBXMeshData& mesh_data)
{
	// 頂点バッファの取得
	FbxVector4* vertices = mesh->GetControlPoints();
	// インデックスバッファの取得
	int* indices = mesh->GetPolygonVertices();
	// 頂点座標の数の取得
	int polygon_vertex_count = mesh->GetPolygonVertexCount();

	for (int i = 0; i < polygon_vertex_count; i++)
	{
		FBXVertex vertex{};
		// インデックスバッファから頂点番号を取得
		int index = indices[i];

		// 頂点座標の取得
		vertex.pos[0] = static_cast<float>(-vertices[index][0]); // 左手系に変換
		vertex.pos[1] = static_cast<float>(vertices[index][1]);
		vertex.pos[2] = static_cast<float>(vertices[index][2]);

		// 頂点テーブルに追加
		mesh_data.vertices.push_back(vertex);
	}
}

void ModelLoader::LoadIndices(FbxMesh* mesh, FBXMeshData& mesh_data)
{
	// ポリゴン数の取得
	int polygon_count = mesh->GetPolygonCount();

	for (int i = 0; i < polygon_count; i++)
	{
		mesh_data.indices.push_back(i * 3 + 2);
		mesh_data.indices.push_back(i * 3 + 1);
		mesh_data.indices.push_back(i * 3);
	}
}

void ModelLoader::LoadNormals(FbxMesh* mesh, FBXMeshData& mesh_data)
{
	// 法線リストの取得
	FbxArray<FbxVector4> normals;
	mesh->GetPolygonVertexNormals(normals);

	for (int i = 0; i < normals.Size(); i++)
	{
		// 法線の取得
		FbxVector4 normal = normals[i];

		// 頂点テーブルに追加
		mesh_data.vertices[i].normal[0] = static_cast<float>(-normal[0]); // 左手系に変換
		mesh_data.vertices[i].normal[1] = static_cast<float>(normal[1]);
		mesh_data.vertices[i].normal[2] = static_cast<float>(normal[2]);
	}
}

void ModelLoader::LoadMaterial(FbxSurfaceMaterial* material, FBXModel* model)
{
	FBXMaterial entry_material;
	enum class MaterialType
	{
		Ambient,
		Diffuse,
		//Specular,
		MaxOrder,
	};

	FbxDouble3 colors[(int)MaterialType::MaxOrder];
	FbxDouble factors[(int)MaterialType::MaxOrder];

	// 初期化
	FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sAmbient);

	if (material->GetClassId().Is(FbxSurfaceLambert::ClassId))
	{
		const char* element_check_list[] =
		{
			FbxSurfaceMaterial::sAmbient,
			FbxSurfaceMaterial::sDiffuse,
			// FbxSurfaceMaterial::sSpecular,
		};

		const char* factor_check_list[] =
		{
			FbxSurfaceMaterial::sAmbientFactor,
			FbxSurfaceMaterial::sDiffuseFactor,
			// FbxSurfaceMaterial::sSpecularFactor,
		};

		for (int i = 0; i < (int)MaterialType::MaxOrder; i++)
		{
			prop = material->FindProperty(element_check_list[i]);
			if (prop.IsValid())
			{
				colors[i] = prop.Get<FbxDouble3>();
			}
			else
			{
				colors[i] = FbxDouble3(1.0, 1.0, 1.0);
			}

			prop = material->FindProperty(factor_check_list[i]);
			if (prop.IsValid())
			{
				factors[i] = prop.Get<FbxDouble>();
			}
			else
			{
				factors[i] = 1.0;
			}
		}
	}

	FbxDouble3 color = colors[(int)MaterialType::Ambient];
	FbxDouble factor = factors[(int)MaterialType::Ambient];

	entry_material.ambient[0] = static_cast<float>(color[0]);
	entry_material.ambient[1] = static_cast<float>(color[1]);
	entry_material.ambient[2] = static_cast<float>(color[2]);
	entry_material.ambient[3] = static_cast<float>(factor);

	color = colors[(int)MaterialType::Diffuse];
	factor = factors[(int)MaterialType::Diffuse];

	entry_material.diffuse[0] = static_cast<float>(color[0]);
	entry_material.diffuse[1] = static_cast<float>(color[1]);
	entry_material.diffuse[2] = static_cast<float>(color[2]);
	entry_material.diffuse[3] = static_cast<float>(factor);

	model->material_table[material->GetName()] = entry_material;
	// 定数バッファを初期化しておく
	model->material_const_buffer_table[material->GetName()] = nullptr;
	// ディスクリプタヒープを初期化しておく
	model->material_cbv_heap_table[material->GetName()] = nullptr;
}

void ModelLoader::SetMaterialName(FbxMesh* mesh, FBXMeshData& mesh_data)
{
	// マテリアルが無ければ終わり
	if (mesh->GetElementMaterialCount() == 0)
	{
		mesh_data.material_name = "";
		return;
	}

	// Mesh側のマテリアル情報を取得
	FbxLayerElementMaterial* material = mesh->GetElementMaterial(0);
	int index = material->GetIndexArray().GetAt(0);
	FbxSurfaceMaterial* surface_material = mesh->GetNode()->GetSrcObject<FbxSurfaceMaterial>(index);

	if (surface_material != nullptr)
	{
		mesh_data.material_name = surface_material->GetName();
	}
	else
	{
		mesh_data.material_name = "";
	}
}