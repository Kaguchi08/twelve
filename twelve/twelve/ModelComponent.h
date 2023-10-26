#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "Component.h"
#include "Dx12Wrapper.h"
#include "Model.h"

using Microsoft::WRL::ComPtr;

enum AnimationType {
    Idle,
    Run,
    Max
};

class ModelComponent : public Component {
   public:
    ModelComponent(class Actor* owner, const char* file_name, int draw_order = 100);
    ~ModelComponent();

    void Update(float delta_time) override;

    void DrawPMD(bool is_shadow);

    void AddAnimation(const char* file_name, const AnimationType& name, bool is_loop = true);
    void DeleteAnimation(AnimationType);

    AnimationType GetCurrentAnimation() const { return current_animation_; }

    void SetCurrentAnimation(const AnimationType& name) { current_animation_ = name; }

   private:
    std::shared_ptr<Dx12Wrapper> dx12_;
    std::shared_ptr<class Renderer> renderer_;

    // ���f��
    std::shared_ptr<PMDModel> pmd_model_;

    DirectX::XMMATRIX* mapped_matrices_;

    // �{�[���s��
    std::vector<DirectX::XMMATRIX> bone_matrices_;

    // ���W�ϊ�
    ComPtr<ID3D12Resource> transform_const_buffer_ = nullptr;
    ComPtr<ID3D12DescriptorHeap> transform_cbv_heap_ = nullptr;

    HRESULT CreateTransformResourceAndView();

    // ���[�V����
    void MotionUpdate(float delta_time);
    void RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat);

    std::unordered_map<AnimationType, Animation> animations_;
    AnimationType current_animation_;
    float animation_time_ = 0.0f;

    // IK�֘A
    /// <summary>
    /// CCD-IK�ɂ��{�[������������
    /// </summary>
    /// <param name="ik"></param>
    void SolveCCDIK(const PMDIK& ik);

    /// <summary>
    /// �]���藝IK�ɂ��{�[������������
    /// </summary>
    /// <param name="ik"></param>
    void SolveCosineIK(const PMDIK& ik);

    /// <summary>
    /// LookAt�s��ɂ��{�[������������
    /// </summary>
    /// <param name="ik"></param>
    void SolveLookAt(const PMDIK& ik);

    void IKSolve(int frame_no);
};