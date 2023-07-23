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
	ModelComponent(class Actor* owner, const char* file_name, int draw_order = 100);
	~ModelComponent();

	void Update(float delta_time) override;

	void Draw();
private:
	std::shared_ptr<Dx12Wrapper> dx12_;
	std::shared_ptr<class Renderer> renderer_;

	// モデル
	std::shared_ptr<PMDModel> model_;

	DirectX::XMMATRIX* mapped_matrices_;

	// ボーン行列
	std::vector<DirectX::XMMATRIX> bone_matrices_;

	// 座標変換
	ComPtr<ID3D12Resource> transform_buffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> transform_heap_ = nullptr;

	HRESULT CreateTransformResourceAndView();

	// テスト
	float angle_ = 0.0f;
	DirectX::XMFLOAT3 position_ = { 0.0f, 0.0f, 0.0f };
};