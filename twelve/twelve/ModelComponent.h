#pragma once
#include "Component.h"
#include "Dx12Wrapper.h"
#include "Model.h"
#include <memory>
#include <DirectXMath.h>
#include <wrl.h>
#include <d3d12.h>

using Microsoft::WRL::ComPtr;

class ModelComponent : public Component
{
public:
	ModelComponent(class Actor* owner, ModelType type, const char* file_name, int draw_order = 100);
	~ModelComponent();

	void Update(float delta_time) override;

	void DrawPMD();
	void DrawFBX();

	unsigned int AddAnimation(const char* file_name, bool is_loop = true);
	void DeleteAnimation(unsigned int idx);

private:
	std::shared_ptr<Dx12Wrapper> dx12_;
	std::shared_ptr<class Renderer> renderer_;

	// ���f��
	std::shared_ptr<PMDModel> pmd_model_;
	std::shared_ptr<FBXModel> fbx_model_;
	ModelType type_;

	DirectX::XMMATRIX* mapped_matrices_;

	// �{�[���s��
	std::vector<DirectX::XMMATRIX> bone_matrices_;

	// ���W�ϊ�
	ComPtr<ID3D12Resource> transform_const_buffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> transform_cbv_heap_ = nullptr;

	HRESULT CreateTransformResourceAndView();

	// FBX���f���̃}�e���A���֘A
	HRESULT CreateMaterialResourceAndView(std::string material_name);

	// ���[�V����
	void MotionUpdate(float delta_time);
	void RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat);


	std::unordered_map<unsigned int, Animation> animations_;


	unsigned int current_animation_idx_ = 0;
	unsigned int animation_idx_ = 0;
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

	// �e�X�g
	float angle_ = 0.0f;
	DirectX::XMFLOAT3 position_ = { 0.0f, 0.0f, 0.0f };
};