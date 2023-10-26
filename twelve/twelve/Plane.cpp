#include "Plane.h"

#include <d3dx12.h>

#include <cassert>

Plane::Plane() {
}

Plane::~Plane() {
}

bool Plane::CreateVertexBuffer(ID3D12Device* device) {
    std::vector<PrimitiveVertex> vertices(4);

    // 左下
    vertices[0].pos = {-128.0f, -0.1f, -128.0f};
    vertices[0].normal = {0.0f, 1.0f, 0.0f};
    vertices[0].uv = {0.0f, 0.0f};
    vertices[0].tangent = {1.0f, 0.0f, 0.0f};
    vertices[0].binormal = {0.0f, 0.0f, 1.0f};

    // 右下
    vertices[1].pos = {128.0f, -0.1f, -128.0f};
    vertices[1].normal = {0.0f, 1.0f, 0.0f};
    vertices[1].uv = {1.0f, 0.0f};
    vertices[1].tangent = {1.0f, 0.0f, 0.0f};
    vertices[1].binormal = {0.0f, 0.0f, 1.0f};

    // 右上
    vertices[2].pos = {128.0f, -0.1f, 128.0f};
    vertices[2].normal = {0.0f, 1.0f, 0.0f};
    vertices[2].uv = {1.0f, 1.0f};
    vertices[2].tangent = {1.0f, 0.0f, 0.0f};
    vertices[2].binormal = {0.0f, 0.0f, 1.0f};

    // 左上
    vertices[3].pos = {-128.0f, -0.1f, 128.0f};
    vertices[3].normal = {0.0f, 1.0f, 0.0f};
    vertices[3].uv = {0.0f, 1.0f};
    vertices[3].tangent = {1.0f, 0.0f, 0.0f};
    vertices[3].binormal = {0.0f, 0.0f, 1.0f};

    vertex_num_ = vertices.size();

    auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(vertex_num_ * sizeof(PrimitiveVertex));

    auto result = device->CreateCommittedResource(
        &heap_prop,
        D3D12_HEAP_FLAG_NONE,
        &buffer_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertex_buffer_));

    if (FAILED(result)) {
        assert(0);
        return false;
    }

    // バッファへの書き込み
    PrimitiveVertex* map = nullptr;
    result = vertex_buffer_->Map(0, nullptr, (void**)&map);
    if (FAILED(result)) {
        assert(0);
        return false;
    }

    std::copy(std::begin(vertices), std::end(vertices), map);

    vertex_buffer_->Unmap(0, nullptr);

    // 頂点バッファビューの設定
    vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
    vertex_buffer_view_.SizeInBytes = vertex_num_ * sizeof(PrimitiveVertex);
    vertex_buffer_view_.StrideInBytes = sizeof(PrimitiveVertex);

    return true;
}

bool Plane::CreateIndexBuffer(ID3D12Device* device) {
    unsigned short indices[] = {
        0,
        1,
        3,
        3,
        1,
        2,
    };

    index_num_ = sizeof(indices) / sizeof(indices[0]);

    auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices));

    auto result = device->CreateCommittedResource(
        &heap_prop,
        D3D12_HEAP_FLAG_NONE,
        &buffer_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&index_buffer_));

    if (FAILED(result)) {
        assert(0);
        return false;
    }

    // バッファへの書き込み
    unsigned short* map = nullptr;
    result = index_buffer_->Map(0, nullptr, (void**)&map);
    if (FAILED(result)) {
        assert(0);
        return false;
    }

    std::copy(std::begin(indices), std::end(indices), map);

    index_buffer_->Unmap(0, nullptr);

    // インデックスバッファビューの設定
    index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
    index_buffer_view_.SizeInBytes = sizeof(indices);
    index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;

    return true;
}

bool Plane::CreateTextureAndView(Dx12Wrapper* dx12, const char* file_name) {
    texture_resource_ = dx12->GetResourceManager()->GetTextureFromPath(file_name);

    if (texture_resource_ == nullptr) {
        assert(0);
        return false;
    }

    // ディスクリプタヒープの作成
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = 1;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NodeMask = 0;

    auto result = dx12->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(texture_srv_heap_.ReleaseAndGetAddressOf()));

    if (FAILED(result)) {
        assert(0);
        return false;
    }

    // ディスクリプタの設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = texture_resource_->GetDesc().Format;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    dx12->GetDevice()->CreateShaderResourceView(texture_resource_.Get(), &srv_desc, texture_srv_heap_->GetCPUDescriptorHandleForHeapStart());

    return true;
}

bool Plane::CreateNormalMapAndView(Dx12Wrapper* dx12, const char* file_name) {
    normal_map_resource_ = dx12->GetResourceManager()->GetTextureFromPath(file_name);

    if (normal_map_resource_ == nullptr) {
        assert(0);
        return false;
    }

    // ディスクリプタヒープの作成
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = 1;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NodeMask = 0;

    auto result = dx12->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(normal_map_srv_heap_.ReleaseAndGetAddressOf()));

    if (FAILED(result)) {
        assert(0);
        return false;
    }

    // ディスクリプタの設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = normal_map_resource_->GetDesc().Format;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    dx12->GetDevice()->CreateShaderResourceView(normal_map_resource_.Get(), &srv_desc, normal_map_srv_heap_->GetCPUDescriptorHandleForHeapStart());

    return true;
}

bool Plane::CreateArmMapAndView(Dx12Wrapper* dx12, const char* file_name) {
    arm_map_resource_ = dx12->GetResourceManager()->GetTextureFromPath(file_name);

    if (arm_map_resource_ == nullptr) {
        assert(0);
        return false;
    }

    // ディスクリプタヒープの作成
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = 1;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NodeMask = 0;

    auto result = dx12->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(arm_map_srv_heap_.ReleaseAndGetAddressOf()));

    if (FAILED(result)) {
        assert(0);
        return false;
    }

    // ディスクリプタの設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = arm_map_resource_->GetDesc().Format;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    dx12->GetDevice()->CreateShaderResourceView(arm_map_resource_.Get(), &srv_desc, arm_map_srv_heap_->GetCPUDescriptorHandleForHeapStart());

    return true;
}
