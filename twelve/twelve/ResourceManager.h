#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <map>
#include <memory>
#include <unordered_map>
#include <DirectXTex.h>
#include <wrl.h>
#include <string>
#include <functional>
#include "Model.h"

using namespace Microsoft::WRL;

class PMDModel;
class FBXModel;
class VMDAnimation;
class ModelLoader;

class ResourceManager
{
public:
	ResourceManager(ComPtr<ID3D12Device> dev, std::unique_ptr<class ModelLoader> model_loader);

	ComPtr<ID3D12Resource> GetTextureFromPath(const char* tex_path);
	std::shared_ptr<PMDModel> LoadPMDModel(const char* file_path);
	std::shared_ptr<FBXModel> LoadFBXModel(const char* file_path);
	std::shared_ptr<VMDAnimation> LoadVMDAnimation(const char* file_path);


private:
	ComPtr<ID3D12Device> dev_ = nullptr;
	std::unique_ptr<class ModelLoader> model_loader_;
	std::unordered_map<std::string, ComPtr<ID3D12Resource>> teexture_table_;
	std::unordered_map<std::string, std::shared_ptr<PMDModel>> pmd_model_table_;
	std::unordered_map<std::string, std::shared_ptr<FBXModel>> fbx_model_table_;
	std::unordered_map<std::string, std::shared_ptr<VMDAnimation>> vmd_animation_table_;

	// ロード用テーブル
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t> mLoadLambdaTable;

	ID3D12Resource* CreateTextureFromFile(const char* tex_path);
	void CreateTextureLoaderTable();
};