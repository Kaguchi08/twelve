#include "Checkerboard.h"

Checkerboard::Checkerboard() {
}

Checkerboard::~Checkerboard() {
}

bool Checkerboard::CreateVertexBuffer(ID3D12Device* device) {
    return false;
}

bool Checkerboard::CreateIndexBuffer(ID3D12Device* device) {
    return false;
}

bool Checkerboard::CreateTextureAndView(Dx12Wrapper* dx12, const char* file_name) {
    return false;
}

bool Checkerboard::CreateNormalMapAndView(Dx12Wrapper* dx12, const char* file_name) {
    return false;
}

bool Checkerboard::CreateArmMapAndView(Dx12Wrapper* dx12, const char* file_name) {
    return false;
}

std::vector<PrimitiveVertex> Checkerboard::CreateCheckerboardVertices(int width, int height) {
    return std::vector<PrimitiveVertex>();
}

std::vector<unsigned short> Checkerboard::CreateCheckerboardIndices(int width, int height) {
    return std::vector<unsigned short>();
}
