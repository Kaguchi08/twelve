#pragma once
#include "Component.h"
#include "Dx12Wrapper.h"
#include "Model.h"
#include <memory>
#include <DirectXMath.h>
#include <wrl.h>
#include <d3d12.h>

using Microsoft::WRL::ComPtr;

class FBXComponent : public Component
{
public:
	FBXComponent(class Actor* owner, const char* file_name, int draw_order = 100);
	~FBXComponent();

	void Update(float delta_time) override;

	void Draw(bool is_shadow);


private:
	std::shared_ptr<Dx12Wrapper> dx12_;
	std::shared_ptr<class Renderer> renderer_;

	// ���f��
	std::shared_ptr<FBXModel> fbx_model_;

	// ���W�ϊ�
	DirectX::XMMATRIX* world_matrix_ = nullptr;
	ComPtr<ID3D12Resource> transform_const_buffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> transform_cbv_heap_ = nullptr;

	HRESULT CreateTransformResourceAndView();

	// FBX���f���̃}�e���A���֘A
	HRESULT CreateMaterialResourceAndView(std::string material_name);
	// FBX���f���̃e�N�X�`���֘A
	HRESULT CreateTextureView(std::string material_name);
};