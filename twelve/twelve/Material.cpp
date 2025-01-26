#include "Material.h"

#include "FileUtil.h"
#include "Logger.h"

namespace
{
	constexpr const wchar_t* DummyTag = L"";
}

Material::Material()
	: m_pDevice(nullptr)
	, m_pPool(nullptr)
{
}

Material::~Material()
{
	Term();
}

bool Material::Init(ID3D12Device* pDevice, DescriptorPool* pPool, size_t bufferSize, size_t count)
{
	if (pDevice == nullptr || pPool == nullptr || count == 0)
	{
		ELOG("Error : Invalid Argument.");
		return false;
	}

	Term();

	m_pDevice = pDevice;
	m_pDevice->AddRef();

	m_pPool = pPool;
	m_pPool->AddRef();

	m_Subsets.resize(count);

	// ダミーテクスチャを生成
	{
		auto pTexture = new(std::nothrow) Texture();
		if (pTexture == nullptr)
		{
			ELOG("Error : Out of Memory.");
			return false;
		}

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = 1;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		if (!pTexture->Init(pDevice, pPool, &desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false))
		{
			ELOG("Error : Texture::Init() Failed.");
			pTexture->Term();
			delete pTexture;

			return false;
		}

		m_pTextures[DummyTag] = pTexture;
	}

	auto size = bufferSize * count;
	if (size > 0)
	{
		for (size_t i = 0; i < m_Subsets.size(); ++i)
		{
			auto pBuffer = new(std::nothrow) ConstantBuffer();
			if (pBuffer == nullptr)
			{
				ELOG("Error : Out of Memory.");
				return false;
			}

			if (!pBuffer->Init(pDevice, pPool, bufferSize))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");

				return false;
			}

			m_Subsets[i].pConstantBuffer = pBuffer;
			for (auto j = 0; j < TEXTURE_USAGE_COUNT; ++j)
			{
				m_Subsets[i].TextureHandle[j].ptr = 0;
			}
		}
	}
	else
	{
		for (size_t i = 0; i < m_Subsets.size(); ++i)
		{
			m_Subsets[i].pConstantBuffer = nullptr;
			for (auto j = 0; j < TEXTURE_USAGE_COUNT; ++j)
			{
				m_Subsets[i].TextureHandle[j].ptr = 0;
			}
		}
	}

	return true;
}

void Material::Term()
{
	for (auto& itr : m_pTextures)
	{
		itr.second->Term();
		delete itr.second;
		itr.second = nullptr;
	}

	for (size_t i = 0; i < m_Subsets.size(); ++i)
	{
		if (m_Subsets[i].pConstantBuffer != nullptr)
		{
			m_Subsets[i].pConstantBuffer->Term();
			delete m_Subsets[i].pConstantBuffer;
			m_Subsets[i].pConstantBuffer = nullptr;
		}
	}

	m_pTextures.clear();
	m_Subsets.clear();

	if (m_pDevice != nullptr)
	{
		m_pDevice->Release();
		m_pDevice = nullptr;
	}

	if (m_pPool != nullptr)
	{
		m_pPool->Release();
		m_pPool = nullptr;
	}
}

bool Material::SetTexture(size_t index, TEXTURE_USAGE usage, const std::wstring& path, DirectX::ResourceUploadBatch& batch)
{
	if (index >= GetCount())
	{
		ELOG("Error : Invalid Argument.");
		return false;
	}

	// 既に登録済みかチェック
	if (m_pTextures.find(path) != m_pTextures.end())
	{
		m_Subsets[index].TextureHandle[usage] = m_pTextures[path]->GetHandleGPU();
		return true;
	}

	// ファイルパスが存在するかチェック
	std::wstring findPath;
	if (!SearchFilePathW(path.c_str(), findPath))
	{
		m_Subsets[index].TextureHandle[usage] = m_pTextures[DummyTag]->GetHandleGPU();
		return true;
	}

	// ファイル名であることをチェック
	if (PathIsDirectoryW(findPath.c_str()) != FALSE)
	{
		m_Subsets[index].TextureHandle[usage] = m_pTextures[DummyTag]->GetHandleGPU();
		return true;
	}

	// インスタンス生成
	auto pTexture = new(std::nothrow) Texture();
	if (pTexture == nullptr)
	{
		ELOG("Error : Out of Memory.");
		return false;
	}

	auto isSRGB = (TU_BASE_COLOR == usage) || (TU_DIFFUSE == usage) || (TU_SPECULAR == usage);

	// 初期化
	if (!pTexture->Init(m_pDevice, m_pPool, findPath.c_str(), isSRGB, batch))
	{
		ELOG("Error : Texture::Init() Failed.");
		pTexture->Term();
		delete pTexture;

		return false;
	}

	// 登録
	m_pTextures[path] = pTexture;
	m_Subsets[index].TextureHandle[usage] = pTexture->GetHandleGPU();

	return true;
}

void* Material::GetBufferPtr(size_t index) const
{
	if (index >= GetCount())
	{
		return nullptr;
	}

	return m_Subsets[index].pConstantBuffer->GetPtr();
}

D3D12_GPU_VIRTUAL_ADDRESS Material::GetBufferAddress(size_t index) const
{
	if (index >= GetCount())
	{
		return D3D12_GPU_VIRTUAL_ADDRESS();
	}

	return m_Subsets[index].pConstantBuffer->GetAddress();
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetTextureHandle(size_t index, TEXTURE_USAGE usage) const
{
	if (index >= GetCount())
	{
		return D3D12_GPU_DESCRIPTOR_HANDLE();
	}

	return m_Subsets[index].TextureHandle[usage];
}

size_t Material::GetCount() const
{
	return m_Subsets.size();
}
