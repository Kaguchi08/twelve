#pragma once
#include <d3d12.h>
#include <fbxsdk.h>
#include <wrl.h>

#include <unordered_map>
#include <vector>

#include "Model.h"

using Microsoft::WRL::ComPtr;

class ModelLoader {
   public:
    ModelLoader(class Renderer& renderer);
    ~ModelLoader();

    bool LoadPMDModel(const char* file_name, struct PMDModel* model);
    bool LoadFBXModel(const char* file_name, struct FBXModel* model);
    bool LoadVMDFile(const char* file_name, struct VMDAnimation* anim);

   private:
    class Dx12Wrapper* dx12_;
    class Renderer& renderer_;

    HRESULT CreatePMDMaterialData(struct PMDModel* model);
    HRESULT CreatePMDMaterialAndView(struct PMDModel* model);

    HRESULT CreateFBXMaterialData(struct FBXModel* model);
    HRESULT CreateFBXMaterialAndView(struct FBXModel* model);

    // 使ってない
    void CollectFBXMeshNode(FbxNode* node, std::unordered_map<std::string, FbxNode*>& table);

    bool CreateFBXMesh(FbxMesh* mesh, struct FBXModel* model);
    bool CreateFBXVertexBuffers(struct FBXModel* model);
    bool CreateFBXIndexBuffers(struct FBXModel* model);

    void LoadVertices(FbxMesh* mesh, FBXMeshData& mesh_data);
    void LoadIndices(FbxMesh* mesh, FBXMeshData& mesh_data);
    void LoadNormals(FbxMesh* mesh, FBXMeshData& mesh_data);
    void LoadUV(FbxMesh* mesh, FBXMeshData& mesh_data);

    void LoadMaterial(FbxSurfaceMaterial* material, FBXModel* model);
    void LoadTexture(FbxSurfaceMaterial* material, FBXModel* model);

    bool CreateTexturePath(FbxFileTexture* texture, std::string& tex_path);

    void SetMaterialName(FbxMesh* mesh, FBXMeshData& mesh_data);
};