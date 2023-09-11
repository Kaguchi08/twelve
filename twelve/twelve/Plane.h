#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include "XMFLOAT_Helper.h"
#include "Primitive.h"
#include "Dx12Wrapper.h"
#include <memory>

struct PrimitiveVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
	DirectX::XMFLOAT3 color;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT3 binormal;
};

class Plane : public Primitive
{
public:
	Plane();
	~Plane();

	bool CreateVertexBuffer(ID3D12Device* device) override;
	bool CreateIndexBuffer(ID3D12Device* device) override;

	bool CreateTextureAndView(Dx12Wrapper* dx12, const char* file_name) override;
	bool CreateNormalMapAndView(Dx12Wrapper* dx12, const char* file_name) override;
	bool CreateArmMapAndView(Dx12Wrapper* dx12, const char* file_name) override;
};