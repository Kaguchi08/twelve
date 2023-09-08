#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include "XMFLOAT_Helper.h"
#include "Primitive.h"
#include "Dx12Wrapper.h"
#include <memory>

struct PlaneVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
};

class Plane : public Primitive
{
public:
	Plane();
	~Plane();

	bool CreateVertexBuffer(ID3D12Device* device) override;
	bool CreateIndexBuffer(ID3D12Device* device) override;

	bool CreateTextureAndView(Dx12Wrapper* dx12, const char* file_name) override;
};