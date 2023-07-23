#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <unordered_map>
#include <fbxsdk.h>

using Microsoft::WRL::ComPtr;

class ModelLoader
{
public:
	ModelLoader(class Renderer& renderer);
	~ModelLoader();

	bool LoadPMDModel(const char* file_name, struct PMDModel* model);
	bool LoadFBXModel(const char* file_name, struct FBXModel* model);
private:
	class Dx12Wrapper& dx12_;
	class Renderer& renderer_;

	HRESULT CreateMaterialData(struct PMDModel* model);
	HRESULT CreateMaterialAndView(struct PMDModel* model);

	void CollectMeshNode(FbxNode* node, std::unordered_map<std::string, FbxNode*>& table);
	bool CreateMesh(const char* node_name, FbxMesh* mesh);
};