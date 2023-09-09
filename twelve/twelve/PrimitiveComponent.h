#pragma once
#include "Component.h"
#include "Primitive.h"
#include <d3d12.h>
#include <memory>
#include "Dx12Wrapper.h"
#include "Plane.h"


enum PrimitiveType
{
	kPlane,
};

class PrimitiveComponent : public Component
{
public:
	PrimitiveComponent(class Actor* owner, PrimitiveType type, int draw_order = 100);
	~PrimitiveComponent();

	void Draw(bool is_shadow);

	PrimitiveType GetType() const { return type_; }
	std::shared_ptr<Primitive> GetPrimitive() const { return primitive_; }

private:
	std::shared_ptr<Dx12Wrapper> dx12_;
	std::shared_ptr<class Renderer> renderer_;

	PrimitiveType type_;

	std::shared_ptr<Primitive> primitive_;
};