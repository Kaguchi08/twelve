#pragma once

#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h> 
#include <DirectXMath.h>

#include "ComPtr.h"
#include "Constants.h"
#include "DescriptorPool.h"
#include "RenderTarget.h"
#include "DepthTarget.h"
#include "CommandList.h"
#include "Fence.h"
#include "Mesh.h"
#include "ConstantBuffer.h"
#include "Texture.h"
#include "Material.h"
#include "InlineUtil.h"

class D3D12Wrapper
{
public:
	D3D12Wrapper();
	~D3D12Wrapper();

	bool Initialize(HWND hWind);
	void Terminate();
	void Render();

	bool InitializeGraphicsPipeline();
	void ReleaseGraphicsResources();

private:
	enum POOL_TYPE
	{
		POOL_TYPE_RES = 0, // CBV / SRV / UAV
		POOL_TYPE_SMP,     // Sampler
		POOL_TYPE_RTV,     // RTV
		POOL_TYPE_DSV,     // DSView

		POOL_COUNT
	};

	HWND								m_hWnd;

	ComPtr<IDXGIFactory4>				m_pFactory;
	ComPtr<ID3D12Device>                m_pDevice;
	ComPtr<ID3D12CommandQueue>          m_pQueue;
	ComPtr<IDXGISwapChain3>             m_pSwapChain;

	RenderTarget						m_RenderTarget[Constants::FrameCount];
	DepthTarget							m_DepthTarget;
	DescriptorPool* m_pPool[POOL_COUNT];
	CommandList							m_CommandList;
	Fence								m_Fence;

	uint32_t                            m_FrameIndex;
	D3D12_VIEWPORT						m_Viewport;
	D3D12_RECT							m_Scissor;

	std::vector<Mesh*>					m_pMeshes;
	std::vector<ConstantBuffer*>		m_pTransforms;
	Material							m_Material;
	ComPtr<ID3D12PipelineState>			m_pPSO;
	ComPtr<ID3D12RootSignature>         m_pRootSignature;

	float								m_RotateAngle;

	void InitializeDebug();
	void Present(uint32_t interval);
};
