#pragma once

#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <d3dcompiler.h> 
#include <DirectXMath.h>

#include "Constants.h"

using Microsoft::WRL::ComPtr;

struct alignas(256) Transform
{
	DirectX::XMMATRIX   World;
	DirectX::XMMATRIX   View;
	DirectX::XMMATRIX   Proj;
};


template<typename T>
struct ConstantBufferView
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC Desc;
	D3D12_CPU_DESCRIPTOR_HANDLE     HandleCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE     HandleGPU;
	T* pBuffer;
};

class D3D12Wrapper
{
public:
	D3D12Wrapper();
	~D3D12Wrapper();

	bool Initialize(HWND hWind);
	void Terminate();
	void Render();

	bool InitializeRendering();

private:
	HWND m_hWnd;

	ComPtr<IDXGIFactory4>				m_pFactory;
	ComPtr<ID3D12Device>                m_pDevice;
	ComPtr<ID3D12CommandQueue>          m_pQueue;
	ComPtr<IDXGISwapChain3>             m_pSwapChain;
	ComPtr<ID3D12Resource>              m_pColorBuffer[Constants::FrameCount];
	ComPtr<ID3D12Resource>              m_pDepthBuffer;
	ComPtr<ID3D12CommandAllocator>      m_pCmdAllocator[Constants::FrameCount];
	ComPtr<ID3D12GraphicsCommandList>   m_pCmdList;
	ComPtr<ID3D12DescriptorHeap>        m_pHeapRTV;
	ComPtr<ID3D12Fence>                 m_pFence;
	ComPtr<ID3D12DescriptorHeap>        m_pHeapDSV;
	ComPtr<ID3D12DescriptorHeap>		m_pHeapCBV;
	ComPtr<ID3D12Resource>              m_pVB;
	ComPtr<ID3D12Resource>              m_pIB;
	ComPtr<ID3D12Resource>              m_pCB[Constants::FrameCount * 2];
	ComPtr<ID3D12RootSignature>         m_pRootSignature;
	ComPtr<ID3D12PipelineState>         m_pPSO;

	HANDLE                              m_FenceEvent;
	uint64_t                            m_FenceCounter[Constants::FrameCount];
	uint32_t                            m_FrameIndex;
	D3D12_CPU_DESCRIPTOR_HANDLE         m_HandleRTV[Constants::FrameCount];
	D3D12_CPU_DESCRIPTOR_HANDLE			m_HandleDSV;
	D3D12_VERTEX_BUFFER_VIEW			m_VBV;
	D3D12_INDEX_BUFFER_VIEW				m_IBV;
	D3D12_VIEWPORT						m_Viewport;
	D3D12_RECT							m_Scissor;
	ConstantBufferView<Transform>		m_CBV[Constants::FrameCount * 2];
	float								m_RotateAngle;

	void InitializeDebug();
	void WaitGpu();
	void Present(uint32_t interval);
};
