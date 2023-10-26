#pragma once
#include "Primitive.h"

class Checkerboard : public Primitive {
   public:
    Checkerboard();
    ~Checkerboard();

    bool CreateVertexBuffer(ID3D12Device* device) override;
    bool CreateIndexBuffer(ID3D12Device* device) override;

    bool CreateTextureAndView(Dx12Wrapper* dx12, const char* file_name) override;
    bool CreateNormalMapAndView(Dx12Wrapper* dx12, const char* file_name) override;
    bool CreateArmMapAndView(Dx12Wrapper* dx12, const char* file_name) override;

   private:
    std::vector<PrimitiveVertex> CreateCheckerboardVertices(int width, int height);
    std::vector<unsigned short> CreateCheckerboardIndices(int width, int height);
};