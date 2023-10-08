#include "ResourceManager.h"
#include <cassert>
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>
#include "Dx12Wrapper.h"
#include "Game.h"
#include "Helper.h"
#include "ModelLoader.h"

ResourceManager::ResourceManager(ComPtr<ID3D12Device> dev, std::unique_ptr<class ModelLoader> model_loader) :
	dev_(dev),
	model_loader_(std::move(model_loader))
{
	// テクスチャローダ関連の初期化
	CreateTextureLoaderTable();
}

ComPtr<ID3D12Resource> ResourceManager::GetTextureFromPath(const char* tex_path)
{
	auto it = teexture_table_.find(tex_path);
	if (it != teexture_table_.end())
	{
		return it->second;
	}
	else
	{
		return ComPtr<ID3D12Resource>(CreateTextureFromFile(tex_path));
	}
}

std::shared_ptr<PMDModel> ResourceManager::LoadPMDModel(const char* file_path)
{
	auto iter = pmd_model_table_.find(file_path);
	if (iter != pmd_model_table_.end())
	{
		return iter->second;
	}
	else
	{
		auto model = std::make_shared<PMDModel>();
		auto result = model_loader_->LoadPMDModel(file_path, model.get());

		if (!result)
		{
			assert(0);
			return nullptr;
		}

		pmd_model_table_[file_path] = model;
		return model;
	}
}

std::shared_ptr<FBXModel> ResourceManager::LoadFBXModel(const char* file_path)
{
	auto iter = fbx_model_table_.find(file_path);
	if (iter != fbx_model_table_.end())
	{
		return iter->second;
	}
	else
	{
		auto model = std::make_shared<FBXModel>();
		auto result = model_loader_->LoadFBXModel(file_path, model.get());

		if (!result)
		{
			assert(0);
			return nullptr;
		}

		fbx_model_table_[file_path] = model;
		return model;
	}
}

std::shared_ptr<VMDAnimation> ResourceManager::LoadVMDAnimation(const char* file_path)
{
	auto iter = vmd_animation_table_.find(file_path);
	if (iter != vmd_animation_table_.end())
	{
		return iter->second;
	}
	else
	{
		vmd_animation_table_[file_path] = std::make_shared<VMDAnimation>();
		auto result = model_loader_->LoadVMDFile(file_path, vmd_animation_table_[file_path].get());
		if (!result)
		{
			assert(0);
			return nullptr;
		}

		return vmd_animation_table_[file_path];
	}
}

ID3D12Resource* ResourceManager::CreateTextureFromFile(const char* tex_path)
{
	std::string strTexPath = tex_path;
	// WICテクスチャのロード
	DirectX::TexMetadata metadata = {};
	DirectX::ScratchImage scratchImg = {};

	auto wTexPath = GetWideStringFromString(strTexPath);
	auto ext = GetExtension(strTexPath);
	auto result = mLoadLambdaTable[ext](wTexPath, &metadata, scratchImg);

	if (FAILED(result))
	{
		return nullptr;
	}

	auto img = scratchImg.GetImage(0, 0, 0);

	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);

	ID3D12Resource* texBuff = nullptr;
	result = dev_->CreateCommittedResource
	(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texBuff)
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	// テクスチャ用バッファへの書き込み
	result = texBuff->WriteToSubresource
	(
		0,
		nullptr,
		img->pixels,
		img->rowPitch,
		img->slicePitch
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	teexture_table_[tex_path] = texBuff;

	return texBuff;
}

void ResourceManager::CreateTextureLoaderTable()
{
	mLoadLambdaTable["sph"]
		= mLoadLambdaTable["spa"]
		= mLoadLambdaTable["bmp"]
		= mLoadLambdaTable["png"]
		= mLoadLambdaTable["jpg"]
		= [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)
		-> HRESULT
	{
		return DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, meta, img);
	};

	mLoadLambdaTable["tga"]
		= [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)
		-> HRESULT
	{
		return DirectX::LoadFromTGAFile(path.c_str(), meta, img);
	};

	mLoadLambdaTable["dds"]
		= [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)
		-> HRESULT
	{
		return DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, meta, img);
	};
}
