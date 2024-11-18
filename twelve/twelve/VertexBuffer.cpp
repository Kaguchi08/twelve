#include "VertexBuffer.h"

#include "Logger.h"

VertexBuffer::VertexBuffer()
	: m_pBuffer(nullptr)
{
	memset(&m_View, 0, sizeof(m_View));
}

VertexBuffer::~VertexBuffer()
{
	Term();
}

bool VertexBuffer::Init(ID3D12Device* pDevice, size_t size, size_t stride, const void* pInitData)
{
	if (pDevice == nullptr || size == 0 || stride == 0)
	{
		ELOG("Error : Invalid Argument.");
		return false;
	}

	// ヒーププロパティを設定
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask = 1;
	prop.VisibleNodeMask = 1;

	// リソースの設定
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// リソースを生成
	auto hr = pDevice->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pBuffer.GetAddressOf()));
	if (FAILED(hr))
	{
		ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. retcode = 0x%x", hr);
		return false;
	}

	// 頂点バッファビューの設定
	m_View.BufferLocation = m_pBuffer->GetGPUVirtualAddress();
	m_View.StrideInBytes = UINT(stride);
	m_View.SizeInBytes = UINT(size);


	// 初期化データがあれば書き込む
	if (pInitData != nullptr)
	{
		void* ptr = Map();
		if (ptr == nullptr)
		{
			ELOG("Error : VertexBuffer::Map() Failed.");
			return false;
		}

		memcpy(ptr, pInitData, size);

		Unmap();
	}

	return true;
}

void VertexBuffer::Term()
{
	m_pBuffer.Reset();
	memset(&m_View, 0, sizeof(m_View));
}

void* VertexBuffer::Map()
{
	void* ptr = nullptr;
	auto hr = m_pBuffer->Map(0, nullptr, &ptr);
	if (FAILED(hr))
	{
		ELOG("Error : ID3D12Resource::Map() Failed. retcode = 0x%x", hr);
		return nullptr;
	}

	return ptr;
}

void VertexBuffer::Unmap()
{
	m_pBuffer->Unmap(0, nullptr);
}

D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetView() const
{
	return m_View;
}
