#pragma once
#include <d3d12.h>

#include <memory>

#include "Component.h"
#include "Dx12Wrapper.h"
#include "Plane.h"
#include "Primitive.h"

enum PrimitiveType
{
	kPlane,
};

class PrimitiveComponent : public Component
{
public:
	PrimitiveComponent(class Actor* owner, PrimitiveType type);
	~PrimitiveComponent();

	void Update(float delta_time) override;
	void ProcessInput(const InputState& state) override;
	void GenerateOutput() override;

	void Draw(bool is_shadow);

	PrimitiveType GetType() const { return type_; }
	std::shared_ptr<Primitive> GetPrimitive() const { return primitive_; }

private:
	std::shared_ptr<Dx12Wrapper> dx12_;
	std::shared_ptr<class Renderer> renderer_;

	PrimitiveType type_;

	std::shared_ptr<Primitive> primitive_;

	// 座標変換
	DirectX::XMMATRIX* world_matrix_ = nullptr;
	ComPtr<ID3D12Resource> transform_const_buffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> transform_cbv_heap_ = nullptr;

	HRESULT CreateTransformResourceAndView();
};
