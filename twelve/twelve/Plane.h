#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

#include <memory>

#include "Dx12Wrapper.h"
#include "Primitive.h"
#include "XMFLOAT_Helper.h"

class Plane : public Primitive {
   public:
    Plane();
    ~Plane();

    bool CreateVertexBuffer(ID3D12Device* device) override;
    bool CreateIndexBuffer(ID3D12Device* device) override;

    bool CreateTextureAndView(Dx12Wrapper* dx12, const char* file_name) override;
    bool CreateNormalMapAndView(Dx12Wrapper* dx12, const char* file_name) override;
    bool CreateArmMapAndView(Dx12Wrapper* dx12, const char* file_name) override;
};