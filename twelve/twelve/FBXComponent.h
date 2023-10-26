#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

#include <memory>

#include "Component.h"
#include "Dx12Wrapper.h"
#include "Model.h"

using Microsoft::WRL::ComPtr;

class FBXComponent : public Component {
   public:
    FBXComponent(class Actor* owner, const char* file_name, int draw_order = 100);
    ~FBXComponent();

    void Update(float delta_time) override;

    void Draw(bool is_shadow);

    // �e��}�b�v�e�N�X�`���̍쐬
    bool CreateNormalMapAndView(const char* file_name);
    bool CreateArmMapAndView(const char* file_name);

   private:
    std::shared_ptr<Dx12Wrapper> dx12_;
    std::shared_ptr<class Renderer> renderer_;

    // ���f��
    std::shared_ptr<FBXModel> fbx_model_;

    // ���W�ϊ�
    DirectX::XMMATRIX* world_matrix_ = nullptr;
    ComPtr<ID3D12Resource> transform_const_buffer_ = nullptr;
    ComPtr<ID3D12DescriptorHeap> transform_cbv_heap_ = nullptr;

    // �e��}�b�v���\�[�X
    ComPtr<ID3D12Resource> normal_map_resource_ = nullptr;
    ComPtr<ID3D12Resource> arm_map_resource_ = nullptr;

    ComPtr<ID3D12DescriptorHeap> normal_map_srv_heap_ = nullptr;
    ComPtr<ID3D12DescriptorHeap> arm_map_srv_heap_ = nullptr;

    HRESULT CreateTransformResourceAndView();

    // FBX���f���̃}�e���A���֘A
    HRESULT CreateMaterialResourceAndView(std::string material_name);
    // FBX���f���̃e�N�X�`���֘A
    HRESULT CreateTextureView(std::string material_name);

    void CalculateTangentAndBinormal(FBXMeshData& mesh_data);
};