#include "DepthTarget.h"

#include "DescriptorPool.h"
#include "Logger.h"

DepthTarget::DepthTarget()
	: m_pTarget(nullptr)
	, m_pHandle(nullptr)
	, m_pPool(nullptr)
{
}

DepthTarget::~DepthTarget()
{
	Term();
}

bool DepthTarget::Init(ID3D12Device* pDevice, DescriptorPool* pPool, uint32_t width, uint32_t height, DXGI_FORMAT format)
{
	if (pDevice == nullptr || pPool == nullptr || width == 0 || height == 0)
	{
		ELOG("Error : Invalid Argument.");
		return false;
	}

	assert(m_pTarget == nullptr);
	assert(m_pHandle == nullptr);

	// ディスクリプタプールを設定
	m_pPool = pPool;
	m_pPool->AddRef();

	// ディスクリプタハンドルを取得
	m_pHandle = m_pPool->AllocHandle();
	if (m_pHandle == nullptr)
	{
		ELOG("Error : Descriptor Handle is full.");
		return false;
	}

	// ヒーププロパティの設定
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_DEFAULT;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask = 1;
	prop.VisibleNodeMask = 1;

	// リソースの設定
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	// リソースの生成
	auto hr = pDevice->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(m_pTarget.GetAddressOf()));
	if (FAILED(hr))
	{
		ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. retcode = 0x%x", hr);
		return false;
	}

	// デプスステンシルビューの設定
	m_Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	m_Desc.Format = format;
	m_Desc.Texture2D.MipSlice = 0;
	m_Desc.Flags = D3D12_DSV_FLAG_NONE;

	// デプスステンシルビューを生成
	pDevice->CreateDepthStencilView(m_pTarget.Get(), &m_Desc, m_pHandle->HandleCPU);

	return true;
}

void DepthTarget::Term()
{
	m_pTarget.Reset();

	// ディスクリプタハンドルを解放
	if (m_pHandle != nullptr && m_pPool != nullptr)
	{
		m_pPool->FreeHandle(m_pHandle);
		m_pHandle = nullptr;
	}

	// ディスクリプタプールを解放
	if (m_pPool != nullptr)
	{
		m_pPool->Release();
		m_pPool = nullptr;
	}
}

DescriptorHandle* DepthTarget::GetHandle() const
{
	return m_pHandle;
}

ID3D12Resource* DepthTarget::GetResource() const
{
	return m_pTarget.Get();
}

D3D12_RESOURCE_DESC DepthTarget::GetDesc() const
{
	if (m_pTarget != nullptr)
	{
		return m_pTarget->GetDesc();
	}

	return D3D12_RESOURCE_DESC();
}

D3D12_DEPTH_STENCIL_VIEW_DESC DepthTarget::GetViewDesc() const
{
	return m_Desc;
}
