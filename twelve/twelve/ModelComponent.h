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

enum AnimationType
{
	Idle,
	Run,
	Max
};

class ModelComponent : public Component
{
public:
	ModelComponent(class Actor* owner, const char* file_name);
	~ModelComponent();

	void Update(float delta_time) override;
	void ProcessInput(const InputState& state) override {};
	void GenerateOutput() override {};

	void DrawPMD(bool is_shadow);

	void AddAnimation(const char* file_name, const AnimationType& name, bool is_loop = true);
	void DeleteAnimation(AnimationType);

	AnimationType GetCurrentAnimation() const { return current_animation_; }

	void SetCurrentAnimation(const AnimationType& name) { current_animation_ = name; }

private:
	std::shared_ptr<Dx12Wrapper> dx12_;
	std::shared_ptr<class Renderer> renderer_;

	// モデル
	std::shared_ptr<PMDModel> pmd_model_;

	DirectX::XMMATRIX* mapped_matrices_;

	// ボーン行列
	std::vector<DirectX::XMMATRIX> bone_matrices_;

	// 座標変換
	ComPtr<ID3D12Resource> transform_const_buffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> transform_cbv_heap_ = nullptr;

	HRESULT CreateTransformResourceAndView();

	// モーション
	void MotionUpdate(float delta_time);
	void RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat);

	std::unordered_map<AnimationType, Animation> animations_;
	AnimationType current_animation_;
	float animation_time_ = 0.0f;

	// IK関連
	/// <summary>
	/// CCD-IKによりボーン方向を解決
	/// </summary>
	/// <param name="ik"></param>
	void SolveCCDIK(const PMDIK& ik);

	/// <summary>
	/// 余弦定理IKによりボーン方向を解決
	/// </summary>
	/// <param name="ik"></param>
	void SolveCosineIK(const PMDIK& ik);

	/// <summary>
	/// LookAt行列によりボーン方向を解決
	/// </summary>
	/// <param name="ik"></param>
	void SolveLookAt(const PMDIK& ik);

	void IKSolve(int frame_no);
};
