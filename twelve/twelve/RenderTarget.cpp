#include "RenderTarget.h"

#include "DescriptorPool.h"
#include "Logger.h"

RenderTarget::RenderTarget()
	: m_pTarget(nullptr)
	, m_pHandle(nullptr)
	, m_pPool(nullptr)
{
}

RenderTarget::~RenderTarget()
{
	Term();
}

bool RenderTarget::Init(ID3D12Device* pDevice, DescriptorPool* pPool, uint32_t width, uint32_t height, DXGI_FORMAT format)
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
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	// クリアカラー
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.Color[0] = 1.0f;
	clearValue.Color[1] = 1.0f;
	clearValue.Color[2] = 1.0f;
	clearValue.Color[3] = 1.0f;

	// リソースの生成
	auto hr = pDevice->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(m_pTarget.GetAddressOf()));
	if (FAILED(hr))
	{
		ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. retcode = 0x%x", hr);
		return false;
	}

	// レンダーターゲットビューの設定
	m_Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	m_Desc.Format = format;
	m_Desc.Texture2D.MipSlice = 0;
	m_Desc.Texture2D.PlaneSlice = 0;

	// レンダーターゲットビューを生成
	pDevice->CreateRenderTargetView(m_pTarget.Get(), &m_Desc, m_pHandle->HandleCPU);

	return true;
}

bool RenderTarget::InitFromBackBuffer(ID3D12Device* pDevice, DescriptorPool* pPool, uint32_t index, IDXGISwapChain* pSwapChain)
{
	if (pDevice == nullptr || pPool == nullptr || pSwapChain == nullptr)
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

	// バックバッファを取得
	auto hr = pSwapChain->GetBuffer(index, IID_PPV_ARGS(m_pTarget.GetAddressOf()));
	if (FAILED(hr))
	{
		ELOG("Error : IDXGISwapChain::GetBuffer() Failed. retcode = 0x%x", hr);
		return false;
	}

	DXGI_SWAP_CHAIN_DESC desc = {};
	pSwapChain->GetDesc(&desc);

	// レンダーターゲットビューの設定
	m_Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	m_Desc.Format = desc.BufferDesc.Format;
	m_Desc.Texture2D.MipSlice = 0;
	m_Desc.Texture2D.PlaneSlice = 0;

	// レンダーターゲットビューを生成
	pDevice->CreateRenderTargetView(m_pTarget.Get(), &m_Desc, m_pHandle->HandleCPU);

	return true;
}

void RenderTarget::Term()
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

DescriptorHandle* RenderTarget::GetHandle() const
{
	return m_pHandle;
}

ID3D12Resource* RenderTarget::GetResource() const
{
	return m_pTarget.Get();
}

D3D12_RESOURCE_DESC RenderTarget::GetDesc() const
{
	if (m_pTarget != nullptr)
	{
		return m_pTarget->GetDesc();
	}

	return D3D12_RESOURCE_DESC();
}

D3D12_RENDER_TARGET_VIEW_DESC RenderTarget::GetViewDesc() const
{
	return m_Desc;
}
