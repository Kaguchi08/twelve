#include "PMDActor.h"
#include "PMDRenderer.h"
#include "Dx12Wrapper.h"
#include "Helper.h"
#include <d3dx12.h>
#include <algorithm>
#include <sstream>
#include <array>

using namespace Microsoft::WRL;
using namespace DirectX;

namespace
{
	// �{�[�����
	enum class BoneType
	{
		Rotation, // ��]
		RotAndMove, // ��]���ړ�
		IK, // IK
		Undefined, // ����`
		IKChild, // IK�e���{�[��
		RotationChild, // ��]�e���{�[��
		IKDestination, // IK�ڑ���
		Invisible, // �����Ȃ��{�[��
	};
}

PMDActor::PMDActor(const char* filepath, PMDRenderer& renderer)
	: renderer_(renderer)
	, dx12_(renderer.dx12_)
	, mAngle(0.0f)
{
	mTransform.world = DirectX::XMMatrixIdentity();
	LoadPMDFile(filepath);
	CreateTransformView();
	CreateMaterialData();
	CreateMaterialAndTextureView();
}

PMDActor::~PMDActor()
{

}


void PMDActor::Update()
{
	mAngle += 0.001f;
	mMappedMatrices[0] = DirectX::XMMatrixRotationY(mAngle);

	MotionUpdate();
}

void PMDActor::DrawToBackBuffer()
{
	dx12_.GetCommandList()->IASetVertexBuffers(0, 1, &vb_view_);
	dx12_.GetCommandList()->IASetIndexBuffer(&ib_view_);
	dx12_.GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* transHeaps[] = { mTransformHeap.Get() };
	dx12_.GetCommandList()->SetDescriptorHeaps(1, transHeaps);
	dx12_.GetCommandList()->SetGraphicsRootDescriptorTable(1, mTransformHeap->GetGPUDescriptorHandleForHeapStart());

	ID3D12DescriptorHeap* materialHeaps[] = { material_heap.Get() };
	dx12_.GetCommandList()->SetDescriptorHeaps(1, materialHeaps);

	auto materialH = material_heap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	auto cbvSrvIncSize = dx12_.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	for (auto& m : mMaterials)
	{
		dx12_.GetCommandList()->SetGraphicsRootDescriptorTable(2, materialH);
		dx12_.GetCommandList()->DrawIndexedInstanced(m.indices_num, 2, idxOffset, 0, 0);
		materialH.ptr += cbvSrvIncSize;
		idxOffset += m.indices_num;
	}
}

void PMDActor::LoadVMDFile(const char* filepath)
{
	FILE* fp;
	fopen_s(&fp, filepath, "rb");

	fseek(fp, 50, SEEK_SET);

	unsigned int keyFrameNum = 0;
	fread(&keyFrameNum, sizeof(keyFrameNum), 1, fp);

	struct VMDKeyFrame
	{
		char boneName[15];
		unsigned int frame_no;
		DirectX::XMFLOAT3 location;
		DirectX::XMFLOAT4 quaternion;
		unsigned char bezier[64];
	};

	std::vector<VMDKeyFrame> keyFrames(keyFrameNum);

	for (auto& keyFrame : keyFrames)
	{
		fread(keyFrame.boneName, sizeof(keyFrame.boneName), 1, fp);
		fread(&keyFrame.frame_no,
			  sizeof(keyFrame.frame_no)
			  + sizeof(keyFrame.location)
			  + sizeof(keyFrame.quaternion)
			  + sizeof(keyFrame.bezier),
			  1,
			  fp
		);
	}

#pragma pack(1)
	//�\��f�[�^(���_���[�t�f�[�^)
	struct VMDMorph
	{
		char name[15];//���O(�p�f�B���O���Ă��܂�)
		uint32_t frame_no;//�t���[���ԍ�
		float weight;//�E�F�C�g(0.0f�`1.0f)
	};//�S����23�o�C�g�Ȃ̂�pragmapack�œǂ�
#pragma pack()
	uint32_t morphCount = 0;
	fread(&morphCount, sizeof(morphCount), 1, fp);
	std::vector<VMDMorph> morphs(morphCount);
	fread(morphs.data(), sizeof(VMDMorph), morphCount, fp);

#pragma pack(1)
	//�J����
	struct VMDCamera
	{
		uint32_t frame_no; // �t���[���ԍ�
		float distance; // ����
		XMFLOAT3 pos; // ���W
		XMFLOAT3 eulerAngle; // �I�C���[�p
		uint8_t Interpolation[24]; // �⊮
		uint32_t fov; // ���E�p
		uint8_t persFlg; // �p�[�X�t���OON/OFF
	};//61�o�C�g(�����pragma pack(1)�̕K�v����)
#pragma pack()
	uint32_t vmdCameraCount = 0;
	fread(&vmdCameraCount, sizeof(vmdCameraCount), 1, fp);
	std::vector<VMDCamera> cameraData(vmdCameraCount);
	fread(cameraData.data(), sizeof(VMDCamera), vmdCameraCount, fp);

	// ���C�g�Ɩ��f�[�^
	struct VMDLight
	{
		uint32_t frame_no; // �t���[���ԍ�
		XMFLOAT3 rgb; //���C�g�F
		XMFLOAT3 vec; //�����x�N�g��(���s����)
	};

	uint32_t vmdLightCount = 0;
	fread(&vmdLightCount, sizeof(vmdLightCount), 1, fp);
	std::vector<VMDLight> lights(vmdLightCount);
	fread(lights.data(), sizeof(VMDLight), vmdLightCount, fp);

#pragma pack(1)
	// �Z���t�e�f�[�^
	struct VMDSelfShadow
	{
		uint32_t frame_no; // �t���[���ԍ�
		uint8_t mode; //�e���[�h(0:�e�Ȃ��A1:���[�h�P�A2:���[�h�Q)
		float distance; //����
	};
#pragma pack()
	uint32_t selfShadowCount = 0;
	fread(&selfShadowCount, sizeof(selfShadowCount), 1, fp);
	std::vector<VMDSelfShadow> selfShadowData(selfShadowCount);
	fread(selfShadowData.data(), sizeof(VMDSelfShadow), selfShadowCount, fp);

	//IK�I���I�t�؂�ւ�萔
	uint32_t ikSwitchCount = 0;
	fread(&ikSwitchCount, sizeof(ikSwitchCount), 1, fp);

	mIKEnableData.resize(ikSwitchCount);

	for (auto& ikEnable : mIKEnableData)
	{
		//�L�[�t���[�����Ȃ̂ł܂��̓t���[���ԍ��ǂݍ���
		fread(&ikEnable.frame_no, sizeof(ikEnable.frame_no), 1, fp);
		//���ɉ��t���O������܂�������͎g�p���Ȃ��̂�1�o�C�g�V�[�N�ł��\���܂���
		uint8_t visibleFlg = 0;
		fread(&visibleFlg, sizeof(visibleFlg), 1, fp);
		//�Ώۃ{�[�����ǂݍ���
		uint32_t ikBoneCount = 0;
		fread(&ikBoneCount, sizeof(ikBoneCount), 1, fp);
		//���[�v�����O��ON/OFF�����擾
		for (int i = 0; i < ikBoneCount; ++i)
		{
			char ikBoneName[20];
			fread(ikBoneName, _countof(ikBoneName), 1, fp);
			uint8_t flg = 0;
			fread(&flg, sizeof(flg), 1, fp);
			ikEnable.ik_enable_table[ikBoneName] = flg;
		}
	}

	fclose(fp);

	// VMD�̃L�[�t���[���f�[�^����A�g�p����L�[�t���[���e�[�u���֕ϊ�
	for (auto& keyFrame : keyFrames)
	{
		key_frames_[keyFrame.boneName].emplace_back(
			KeyFrame(keyFrame.frame_no,
					 DirectX::XMLoadFloat4(&keyFrame.quaternion),
					 keyFrame.location,
					 DirectX::XMFLOAT2((float)keyFrame.bezier[3] / 127.0f, (float)keyFrame.bezier[7] / 127.0f),
					 DirectX::XMFLOAT2((float)keyFrame.bezier[11] / 127.0f, (float)keyFrame.bezier[15] / 127.0f)
			));

		duration_ = std::max<unsigned int>(duration_, keyFrame.frame_no);
	}

	for (auto& boneMotion : key_frames_)
	{
		std::sort(boneMotion.second.begin(), boneMotion.second.end(),
				  [](const KeyFrame& lval, const KeyFrame& rval)
				  {
					  return lval.frame_no < rval.frame_no;
				  });
	}

	for (auto& boneMotion : key_frames_)
	{
		auto itBoneNode = bone_node_table.find(boneMotion.first);
		if (itBoneNode == bone_node_table.end())
		{
			continue;
		}

		auto node = itBoneNode->second;
		auto& pos = node.start_pos;
		auto mat = DirectX::XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* DirectX::XMMatrixRotationQuaternion(boneMotion.second[0].quaternion)
			* DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
		mBoneMatrices[node.bone_idx] = mat;
	}

	//RecursiveMatrixMultipy(&mBoneNodeTable["�S�Ă̐e"], DirectX::XMMatrixIdentity());
	RecursiveMatrixMultipy(&bone_node_table["�Z���^�["], DirectX::XMMatrixIdentity());

	std::copy(mBoneMatrices.begin(), mBoneMatrices.end(), mMappedMatrices + 1);
}

void PMDActor::PlayAnimation()
{
	mStartTime = timeGetTime();
}

HRESULT PMDActor::CreateMaterialData()
{
	// �}�e���A���o�b�t�@�̍쐬
	auto materialBufferSize = sizeof(MaterialForHlsl);
	materialBufferSize = (materialBufferSize + 0xff) & ~0xff; // 256�̔{���ɂ���

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBufferSize * mMaterials.size());

	auto result = dx12_.GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(material_buffer.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
		return result;
	}

	// �}�e���A���o�b�t�@�̃}�b�v
	char* mapMaterial = nullptr;

	result = material_buffer->Map(0, nullptr, (void**)&mapMaterial);
	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
		return result;
	}

	// �}�e���A���o�b�t�@�ւ̏�������
	for (auto& m : mMaterials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.hlsl;
		mapMaterial += materialBufferSize;
	}

	// �}�e���A���o�b�t�@�̃A���}�b�v
	material_buffer->Unmap(0, nullptr);

	return S_OK;
}

HRESULT PMDActor::CreateMaterialAndTextureView()
{
	D3D12_DESCRIPTOR_HEAP_DESC materialDescHeapDesc = {};
	materialDescHeapDesc.NumDescriptors = mMaterials.size() * 5;
	materialDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	materialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	materialDescHeapDesc.NodeMask = 0;

	auto result = dx12_.GetDevice()->CreateDescriptorHeap(&materialDescHeapDesc, IID_PPV_ARGS(material_heap.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
		return result;
	}

	auto materialBufferSize = sizeof(MaterialForHlsl);
	materialBufferSize = (materialBufferSize + 0xff) & ~0xff; // 256�̔{���ɂ���

	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = material_buffer->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = materialBufferSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE matDescHeapHandle(material_heap->GetCPUDescriptorHandleForHeapStart());
	auto incSize = dx12_.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < mMaterials.size(); ++i)
	{
		dx12_.GetDevice()->CreateConstantBufferView(&matCBVDesc, matDescHeapHandle);
		matDescHeapHandle.ptr += incSize; // Offset()�ł�������
		matCBVDesc.BufferLocation += materialBufferSize;

		if (texture_resources[i] == nullptr)
		{
			srvDesc.Format = renderer_.mWhiteTexture->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(renderer_.mWhiteTexture.Get(), &srvDesc, matDescHeapHandle);
		}
		else
		{
			srvDesc.Format = texture_resources[i]->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(texture_resources[i].Get(), &srvDesc, matDescHeapHandle);
		}
		matDescHeapHandle.ptr += incSize;

		if (sph_resources[i] == nullptr)
		{
			srvDesc.Format = renderer_.mWhiteTexture->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(renderer_.mWhiteTexture.Get(), &srvDesc, matDescHeapHandle);
		}
		else
		{
			srvDesc.Format = sph_resources[i]->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(sph_resources[i].Get(), &srvDesc, matDescHeapHandle);
		}
		matDescHeapHandle.ptr += incSize;

		if (spa_resources[i] == nullptr)
		{
			srvDesc.Format = renderer_.mBlackTexture->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(renderer_.mBlackTexture.Get(), &srvDesc, matDescHeapHandle);
		}
		else
		{
			srvDesc.Format = spa_resources[i]->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(spa_resources[i].Get(), &srvDesc, matDescHeapHandle);
		}
		matDescHeapHandle.ptr += incSize;

		if (toon_resources[i] == nullptr)
		{
			srvDesc.Format = renderer_.mGradTexture->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(renderer_.mGradTexture.Get(), &srvDesc, matDescHeapHandle);
		}
		else
		{
			srvDesc.Format = toon_resources[i]->GetDesc().Format;
			dx12_.GetDevice()->CreateShaderResourceView(toon_resources[i].Get(), &srvDesc, matDescHeapHandle);
		}
		matDescHeapHandle.ptr += incSize;
	}

	return S_OK;
}

HRESULT PMDActor::CreateTransformView()
{
	// GPU�o�b�t�@�̍쐬
	auto bufferSize = sizeof(DirectX::XMMATRIX) * (1 + mBoneMatrices.size()); // wordl�s�� + �{�[���s�� * �{�[����
	bufferSize = (bufferSize + 0xff) & ~0xff; // 256�̔{���ɂ���

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	auto result = dx12_.GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mTransformBuffer.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
		return result;
	}

	// �}�b�v
	result = mTransformBuffer->Map(0, nullptr, (void**)&mMappedMatrices);

	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
		return result;
	}

	mMappedMatrices[0] = mTransform.world;
	std::copy(mBoneMatrices.begin(), mBoneMatrices.end(), mMappedMatrices + 1);

	// �r���[�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC transformDescHeapDesc = {};
	transformDescHeapDesc.NumDescriptors = 1;
	transformDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	transformDescHeapDesc.NodeMask = 0;
	transformDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = dx12_.GetDevice()->CreateDescriptorHeap(&transformDescHeapDesc, IID_PPV_ARGS(mTransformHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
		return result;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = mTransformBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferSize;

	dx12_.GetDevice()->CreateConstantBufferView(&cbvDesc, mTransformHeap->GetCPUDescriptorHandleForHeapStart());

	return S_OK;
}

HRESULT PMDActor::LoadPMDFile(const char* filepath)
{
	// PMD �w�b�_�[�\����
	struct PMDHeader
	{
		float version; // 00 00 80 3F == 1.00
		char modelName[20]; // ���f����
		char comment[256]; // �R�����g
	};

	char signature[3]; // "PMD"

	PMDHeader pmdHeader = {};
	std::string strModelPath = filepath;

	FILE* fp;
	fopen_s(&fp, strModelPath.c_str(), "rb");

	if (fp == nullptr)
	{
		assert(0);
		return ERROR_FILE_NOT_FOUND;
	}

	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	unsigned int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, fp);

#pragma pack(1)
	// PMD�}�e���A���\����
	struct PMDMaterial
	{
		DirectX::XMFLOAT3 diffuse; // �f�B�t���[�Y�F
		float alpha; // �f�B�t���[�Y�̃A���t�@�l
		float specularity; // �X�y�L�����̋���(��Z�l)
		DirectX::XMFLOAT3 specular; // �X�y�L�����F
		DirectX::XMFLOAT3 ambient; // �A���r�G���g�F
		unsigned char toon_idx; // �g�D�[���ԍ�(0�`9)
		unsigned char edge_flg; // �֊s���t���O
		unsigned int indices_num; // �ʒ��_��
		char tex_path[20]; // �e�N�X�`���p�X
	};
#pragma pack()

	constexpr unsigned int pmdVertexSize = 38;
	std::vector<unsigned char> vertices(vertNum * pmdVertexSize);

	fread(vertices.data(), vertices.size(), 1, fp);

	unsigned int indices_num;
	fread(&indices_num, sizeof(indices_num), 1, fp);

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(vertices[0]));

	auto result = dx12_.GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(vertex_buffer_.ReleaseAndGetAddressOf())
	);

	unsigned char* vertMap = nullptr;
	result = vertex_buffer_->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertices.begin(), vertices.end(), vertMap);
	vertex_buffer_->Unmap(0, nullptr);

	vb_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
	vb_view_.SizeInBytes = vertices.size();
	vb_view_.StrideInBytes = pmdVertexSize;

	std::vector<unsigned short> indices(indices_num);

	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	resDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));

	result = dx12_.GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(index_buffer_.ReleaseAndGetAddressOf())
	);

	unsigned short* indexMap = nullptr;
	result = index_buffer_->Map(0, nullptr, (void**)&indexMap);
	std::copy(indices.begin(), indices.end(), indexMap);
	index_buffer_->Unmap(0, nullptr);

	ib_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
	ib_view_.Format = DXGI_FORMAT_R16_UINT;
	ib_view_.SizeInBytes = indices.size() * sizeof(indices[0]);

	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, fp);

	mMaterials.resize(materialNum);
	texture_resources.resize(materialNum);
	sph_resources.resize(materialNum);
	spa_resources.resize(materialNum);
	toon_resources.resize(materialNum);

	std::vector<PMDMaterial> pmdMaterials(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);

	// �R�s�[
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		mMaterials[i].indices_num = pmdMaterials[i].indices_num;
		mMaterials[i].hlsl.diffuse = pmdMaterials[i].diffuse;
		mMaterials[i].hlsl.alpha = pmdMaterials[i].alpha;
		mMaterials[i].hlsl.specular = pmdMaterials[i].specular;
		mMaterials[i].hlsl.specularity = pmdMaterials[i].specularity;
		mMaterials[i].hlsl.ambient = pmdMaterials[i].ambient;
		mMaterials[i].additional.toon_idx = pmdMaterials[i].toon_idx;
	}

	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		//std::string toonFilePath = "../Assets/Model/Kafka/";
		std::string toonFilePath = "../toon/";
		char toonFileName[32];
		//sprintf_s(toonFileName, 32, "toon%d.png", pmdMaterials[i].toonIdx + 1);
		sprintf_s(toonFileName, 32, "toon%02d.bmp", pmdMaterials[i].toon_idx + 1);
		toonFilePath += toonFileName;

		toon_resources[i] = dx12_.GetResourceManager()->GetTextureFromPath(toonFilePath.c_str());

		if (strlen(pmdMaterials[i].tex_path) == 0)
		{
			texture_resources[i] = nullptr;
			continue;
		}

		std::string texFileName = pmdMaterials[i].tex_path;
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
			if (GetExtension(pmdMaterials[i].tex_path) == "sph")
			{
				texFileName = "";
				sphFileName = pmdMaterials[i].tex_path;
			}
			else if (GetExtension(pmdMaterials[i].tex_path) == "spa")
			{
				texFileName = "";
				spaFileName = pmdMaterials[i].tex_path;
			}
			else
			{
				texFileName = pmdMaterials[i].tex_path;
			}
		}

		if (texFileName != "")
		{
			auto texFilePath = GetTexturePathFromModelAndTexPath(strModelPath, texFileName.c_str());
			texture_resources[i] = dx12_.GetResourceManager()->GetTextureFromPath(texFilePath.c_str());
		}

		if (sphFileName != "")
		{
			auto sphFilePath = GetTexturePathFromModelAndTexPath(strModelPath, sphFileName.c_str());
			sph_resources[i] = dx12_.GetResourceManager()->GetTextureFromPath(sphFilePath.c_str());
		}

		if (spaFileName != "")
		{
			auto spaFilePaht = GetTexturePathFromModelAndTexPath(strModelPath, spaFileName.c_str());
			spa_resources[i] = dx12_.GetResourceManager()->GetTextureFromPath(spaFilePaht.c_str());
		}
	}

	// �{�[���̓ǂݍ���
	unsigned short boneNum;
	fread(&boneNum, sizeof(boneNum), 1, fp);

#pragma pack(1)
	// �ǂݎ��pbone�\����
	struct PMDBone
	{
		char boneName[20]; // �{�[����
		unsigned short parentNo; // �e�{�[���ԍ�
		unsigned short nextNo; // ��[�{�[���ԍ�
		unsigned char type; // �{�[���̎��
		unsigned short ikBoneNo; // IK�{�[���ԍ�
		DirectX::XMFLOAT3 pos; // �{�[���̈ʒu
	};
#pragma pack()

	std::vector<PMDBone> pmdBones(boneNum);
	fread(pmdBones.data(), sizeof(PMDBone), boneNum, fp);

	uint16_t ikNum = 0;
	fread(&ikNum, sizeof(ikNum), 1, fp);

	ik_data.resize(ikNum);

	for (auto& ik : ik_data)
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

	bone_name_array.resize(pmdBones.size());
	bone_node_address_array.resize(pmdBones.size());

	knee_idxes.clear();

	// �{�[���m�[�h�}�b�v�̍쐬
	for (int idx = 0; idx < pmdBones.size(); ++idx)
	{
		auto& pb = pmdBones[idx];
		auto& node = bone_node_table[pb.boneName];
		node.bone_idx = idx;
		node.start_pos = pb.pos;
		node.bone_type = pb.type;
		node.parent_bone = pb.parentNo;
		node.ik_parent_bone = pb.ikBoneNo;

		bone_name_array[idx] = pb.boneName;
		bone_node_address_array[idx] = &node;

		std::string boneName = pb.boneName;
		if (boneName.find("�Ђ�") != std::string::npos)
		{
			knee_idxes.emplace_back(idx);
		}
	}

	// �{�[���̐e�q�֌W���\�z
	for (auto& pb : pmdBones)
	{
		if (pb.parentNo >= pmdBones.size())
		{
			continue;
		}

		auto paarntName = bone_name_array[pb.parentNo];
		bone_node_table[paarntName].children.emplace_back(&bone_node_table[pb.boneName]);
	}

	mBoneMatrices.resize(pmdBones.size());

	// �{�[���̏�����
	std::fill(mBoneMatrices.begin(), mBoneMatrices.end(), DirectX::XMMatrixIdentity());

	// IK�f�o�b�O�p
	auto getNameFromIdx = [&](uint16_t idx)->std::string
	{
		auto it = std::find_if(bone_node_table.begin(),
							   bone_node_table.end(),
							   [idx](const std::pair<std::string, BoneNode>& pair)
							   {
								   return pair.second.bone_idx == idx;
							   });
		if (it != bone_node_table.end())
		{
			return it->first;
		}
		else
		{
			return "";
		}
	};

	for (auto& ik : ik_data)
	{
		std::ostringstream oss;
		oss << "IK�{�[���ԍ�=" << ik.bone_idx << ":" << getNameFromIdx(ik.bone_idx) << std::endl;
		for (auto& node : ik.node_idxes)
		{
			oss << "\t�m�[�h�{�[��=" << node << ":" << getNameFromIdx(node) << std::endl;
		}

		OutputDebugString(oss.str().c_str());
	}

	return S_OK;
}

void PMDActor::RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat)
{
	mBoneMatrices[node->bone_idx] *= mat;
	for (auto& child : node->children)
	{
		RecursiveMatrixMultipy(child, mBoneMatrices[node->bone_idx]);
	}
}

void PMDActor::MotionUpdate()
{
	auto elapsedTime = timeGetTime() - mStartTime;
	unsigned int frame_no = 30 * elapsedTime / 1000;

	if (frame_no > duration_)
	{
		mStartTime = timeGetTime();
		frame_no = 0;
	}

	// �s��̃N���A
	std::fill(mBoneMatrices.begin(), mBoneMatrices.end(), DirectX::XMMatrixIdentity());

	// ���[�V�����̍X�V
	for (auto& boneMotion : key_frames_)
	{
		auto& boneName = boneMotion.first;
		auto itBoneNode = bone_node_table.find(boneName);

		if (itBoneNode == bone_node_table.end())
		{
			continue;
		}
		auto node = itBoneNode->second;

		auto keyFrames = boneMotion.second;

		auto rit = std::find_if(keyFrames.rbegin(), keyFrames.rend(), [frame_no](const KeyFrame& keyFrame)
								{
									return keyFrame.frame_no <= frame_no;
								});

		if (rit == keyFrames.rend())
		{
			continue;
		}

		// ���
		DirectX::XMMATRIX rotation = DirectX::XMMatrixIdentity();
		DirectX::XMVECTOR offset = DirectX::XMLoadFloat3(&rit->offset);

		auto it = rit.base();

		if (it != keyFrames.end())
		{
			auto nextFrame = it->frame_no;
			auto prevFrame = rit->frame_no;
			auto t = static_cast<float>(frame_no - prevFrame) / static_cast<float>(nextFrame - prevFrame); // ���ۂɂ� x
			t = GetYFromXOnBezier(t, it->p1, it->p2, 12);

			rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMQuaternionSlerp(rit->quaternion, it->quaternion, t));
			offset = DirectX::XMVectorLerp(offset, DirectX::XMLoadFloat3(&it->offset), t);
		}
		else
		{
			rotation = DirectX::XMMatrixRotationQuaternion(rit->quaternion);
		}

		auto& pos = node.start_pos;
		auto mat = DirectX::XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* rotation
			* DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
		mBoneMatrices[node.bone_idx] = mat * DirectX::XMMatrixTranslationFromVector(offset);
	}

	//RecursiveMatrixMultipy(&mBoneNodeTable["�S�Ă̐e"], DirectX::XMMatrixIdentity());
	RecursiveMatrixMultipy(&bone_node_table["�Z���^�["], DirectX::XMMatrixIdentity());

	IKSolve(frame_no);

	std::copy(mBoneMatrices.begin(), mBoneMatrices.end(), mMappedMatrices + 1);
}

void PMDActor::SolveCCDIK(const PMDIK& ik)
{
	//�덷�͈͓̔����ǂ����Ɏg�p����萔
	constexpr float epsilon = 0.0005f;

	//�^�[�Q�b�g
	auto targetNode = bone_node_address_array[ik.bone_idx];
	auto targetOriginPos = DirectX::XMLoadFloat3(&targetNode->start_pos);

	auto parentMat = mBoneMatrices[bone_node_address_array[ik.bone_idx]->ik_parent_bone];
	DirectX::XMVECTOR det;
	auto invParentMat = DirectX::XMMatrixInverse(&det, parentMat);
	auto targetNextPos = DirectX::XMVector3Transform(targetOriginPos, mBoneMatrices[ik.bone_idx] * invParentMat);


	//�܂���IK�̊Ԃɂ���{�[���̍��W�����Ă���(�t������)
	std::vector<DirectX::XMVECTOR> bonePositions;
	//���[�m�[�h
	auto endPos = DirectX::XMLoadFloat3(&bone_node_address_array[ik.target_bone_idx]->start_pos);
	//���ԃm�[�h(���[�g���܂�)
	for (auto& cidx : ik.node_idxes)
	{
		bonePositions.push_back(DirectX::XMLoadFloat3(&bone_node_address_array[cidx]->start_pos));
	}

	std::vector<DirectX::XMMATRIX> mats(bonePositions.size());
	fill(mats.begin(), mats.end(), DirectX::XMMatrixIdentity());
	//������Ƃ悭�킩��Ȃ����APMD�G�f�B�^��6.8����0.03�ɂȂ��Ă���A�����180�Ŋ����������̒l�ł���B
	//�܂肱������W�A���Ƃ��Ďg�p����ɂ�XM_PI����Z���Ȃ���΂Ȃ�Ȃ��c�Ǝv����B
	auto ikLimit = ik.limit * DirectX::XM_PI;
	//ik�ɐݒ肳��Ă��鎎�s�񐔂����J��Ԃ�
	for (int c = 0; c < ik.iterations; ++c)
	{
		//�^�[�Q�b�g�Ɩ��[���قڈ�v�����甲����
		if (DirectX::XMVector3Length(DirectX::XMVectorSubtract(endPos, targetNextPos)).m128_f32[0] <= epsilon)
		{
			break;
		}
		//���ꂼ��̃{�[����k��Ȃ���p�x�����Ɉ����|����Ȃ��悤�ɋȂ��Ă���
		for (int bidx = 0; bidx < bonePositions.size(); ++bidx)
		{
			const auto& pos = bonePositions[bidx];

			//�܂����݂̃m�[�h���疖�[�܂łƁA���݂̃m�[�h����^�[�Q�b�g�܂ł̃x�N�g�������
			auto vecToEnd = DirectX::XMVectorSubtract(endPos, pos);
			auto vecToTarget = DirectX::XMVectorSubtract(targetNextPos, pos);
			vecToEnd = DirectX::XMVector3Normalize(vecToEnd);
			vecToTarget = DirectX::XMVector3Normalize(vecToTarget);

			//�قړ����x�N�g���ɂȂ��Ă��܂����ꍇ�͊O�ςł��Ȃ����ߎ��̃{�[���Ɉ����n��
			if (DirectX::XMVector3Length(DirectX::XMVectorSubtract(vecToEnd, vecToTarget)).m128_f32[0] <= epsilon)
			{
				continue;
			}
			//�O�όv�Z����ъp�x�v�Z
			auto cross = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(vecToEnd, vecToTarget));
			float angle = DirectX::XMVector3AngleBetweenVectors(vecToEnd, vecToTarget).m128_f32[0];
			angle = min(angle, ikLimit);//��]���E�␳
			DirectX::XMMATRIX rot = DirectX::XMMatrixRotationAxis(cross, angle);//��]�s��
			//pos�𒆐S�ɉ�]
			auto mat = DirectX::XMMatrixTranslationFromVector(-pos)
				* rot
				* DirectX::XMMatrixTranslationFromVector(pos);
			mats[bidx] *= mat;//��]�s���ێ����Ă���(��Z�ŉ�]�d�ˊ|��������Ă���)
			//�ΏۂƂȂ�_�����ׂĉ�]������(���݂̓_���猩�Ė��[������])
			for (auto idx = bidx - 1; idx >= 0; --idx)
			{//��������]������K�v�͂Ȃ�
				bonePositions[idx] = DirectX::XMVector3Transform(bonePositions[idx], mat);
			}
			endPos = DirectX::XMVector3Transform(endPos, mat);
			//���������ɋ߂��Ȃ��Ă��烋�[�v�𔲂���
			if (DirectX::XMVector3Length(DirectX::XMVectorSubtract(endPos, targetNextPos)).m128_f32[0] <= epsilon)
			{
				break;
			}
		}
	}
	int idx = 0;
	for (auto& cidx : ik.node_idxes)
	{
		mBoneMatrices[cidx] = mats[idx];
		++idx;
	}
	auto node = bone_node_address_array[ik.node_idxes.back()];
	RecursiveMatrixMultipy(node, parentMat);
}

void PMDActor::SolveCosineIK(const PMDIK& ik)
{
	std::vector<DirectX::XMVECTOR> positions;
	std::array<float, 2> edgeLens;

	auto& targetNode = bone_node_address_array[ik.bone_idx];
	auto targetPos = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&targetNode->start_pos), mBoneMatrices[ik.bone_idx]);

	auto endNode = bone_node_address_array[ik.target_bone_idx];
	positions.emplace_back(DirectX::XMLoadFloat3(&endNode->start_pos));

	for (auto& chainBoneIdx : ik.node_idxes)
	{
		auto boneNode = bone_node_address_array[chainBoneIdx];
		positions.emplace_back(DirectX::XMLoadFloat3(&boneNode->start_pos));
	}

	std::reverse(positions.begin(), positions.end());

	edgeLens[0] = DirectX::XMVector3Length(DirectX::XMVectorSubtract(positions[1], positions[0])).m128_f32[0];
	edgeLens[1] = DirectX::XMVector3Length(DirectX::XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	positions[0] = DirectX::XMVector3Transform(positions[0], mBoneMatrices[ik.node_idxes[1]]);
	positions[2] = DirectX::XMVector3Transform(positions[2], mBoneMatrices[ik.bone_idx]);

	//���[�g�����[�ւ̃x�N�g��������Ă���
	auto linearVec = DirectX::XMVectorSubtract(positions[2], positions[0]);
	float A = DirectX::XMVector3Length(linearVec).m128_f32[0];
	float B = edgeLens[0];
	float C = edgeLens[1];

	linearVec = DirectX::XMVector3Normalize(linearVec);

	//���[�g����^�񒆂ւ̊p�x�v�Z
	float theta1 = acosf((A * A + B * B - C * C) / (2 * A * B));

	//�^�񒆂���^�[�Q�b�g�ւ̊p�x�v�Z
	float theta2 = acosf((B * B + C * C - A * A) / (2 * B * C));

	//�u���v�����߂�
	//�����^�񒆂��u�Ђ��v�ł������ꍇ�ɂ͋����I��X���Ƃ���B
	DirectX::XMVECTOR axis;
	if (std::find(knee_idxes.begin(), knee_idxes.end(), ik.node_idxes[0]) == knee_idxes.end())
	{
		auto vm = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(positions[2], positions[0]));
		auto vt = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(targetPos, positions[0]));
		axis = DirectX::XMVector3Cross(vt, vm);
	}
	else
	{
		auto right = DirectX::XMFLOAT3(1, 0, 0);
		axis = DirectX::XMLoadFloat3(&right);
	}

	//���ӓ_�cIK�`�F�[���͍������Ɍ������Ă��琔�����邽��1���������ɋ߂�
	auto mat1 = DirectX::XMMatrixTranslationFromVector(-positions[0]);
	mat1 *= DirectX::XMMatrixRotationAxis(axis, theta1);
	mat1 *= DirectX::XMMatrixTranslationFromVector(positions[0]);


	auto mat2 = DirectX::XMMatrixTranslationFromVector(-positions[1]);
	mat2 *= DirectX::XMMatrixRotationAxis(axis, theta2 - XM_PI);
	mat2 *= DirectX::XMMatrixTranslationFromVector(positions[1]);

	mBoneMatrices[ik.node_idxes[1]] *= mat1;
	mBoneMatrices[ik.node_idxes[0]] = mat2 * mBoneMatrices[ik.node_idxes[1]];
	mBoneMatrices[ik.target_bone_idx] = mBoneMatrices[ik.node_idxes[0]];
}

void PMDActor::SolveLookAt(const PMDIK& ik)
{
	auto rootNode = bone_node_address_array[ik.node_idxes[0]];
	auto targetNode = bone_node_address_array[ik.target_bone_idx];

	auto rootPos1 = DirectX::XMLoadFloat3(&rootNode->start_pos);
	auto targetPos1 = DirectX::XMLoadFloat3(&targetNode->start_pos);

	auto rootPos2 = DirectX::XMVector3Transform(rootPos1, mBoneMatrices[ik.node_idxes[0]]);
	auto targetPos2 = DirectX::XMVector3Transform(targetPos1, mBoneMatrices[ik.bone_idx]);

	auto originVec = DirectX::XMVectorSubtract(targetPos1, rootPos1);
	auto targetVec = DirectX::XMVectorSubtract(targetPos2, rootPos2);

	originVec = DirectX::XMVector3Normalize(originVec);
	targetVec = DirectX::XMVector3Normalize(targetVec);

	DirectX::XMMATRIX mat = DirectX::XMMatrixTranslationFromVector(-rootPos2)
		* LookAtMatrix(originVec, targetVec, DirectX::XMFLOAT3(0, 1, 0), DirectX::XMFLOAT3(1, 0, 0))
		* DirectX::XMMatrixTranslationFromVector(rootPos2);

	mBoneMatrices[ik.node_idxes[0]] = mat;
}

void PMDActor::IKSolve(int frame_no)
{
	auto it = std::find_if(mIKEnableData.rbegin(), mIKEnableData.rend(),
						   [frame_no](const VMDIKEnable& ikenable)
						   {
							   return ikenable.frame_no <= frame_no;
						   });

	for (auto& ik : ik_data)
	{
		if (it != mIKEnableData.rend())
		{
			auto ikEnableIt = it->ik_enable_table.find(bone_name_array[ik.bone_idx]);
			if (ikEnableIt != it->ik_enable_table.end())
			{
				if (!ikEnableIt->second)
				{//����OFF�Ȃ�ł��؂�܂�
					continue;
				}
			}
		}

		auto childrenNodeCount = ik.node_idxes.size();

		switch (childrenNodeCount)
		{
			case 0:
				assert(0);
				continue;
			case 1:
				SolveLookAt(ik);
				break;
			case 2:
				SolveCosineIK(ik);
				break;
			default:
				SolveCCDIK(ik);
		}
	}
}

float PMDActor::GetYFromXOnBezier(float x, const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b, uint8_t n)
{
	if (a.x == a.y && b.x == b.y)
	{
		return x;
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x;
	const float k1 = 3 * b.x - 6 * a.x;
	const float k2 = 3 * a.x;

	constexpr float epsilon = 0.0005f;

	for (int i = 0; i < n; i++)
	{
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}

		t -= ft / 2;
	}

	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.y + 3 * t * r * r * a.y;
}

void* PMDActor::Transform::operator new(size_t size)
{
	return _aligned_malloc(size, 16);
}
