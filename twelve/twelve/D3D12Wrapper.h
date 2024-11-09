#pragma once

#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h> 

#include "Constants.h"

using Microsoft::WRL::ComPtr;

class D3D12Wrapper
{
public:
	D3D12Wrapper();
	~D3D12Wrapper();

	bool Initialize(HWND hWind);
	void Terminate();
	void Render();

private:
	HWND m_hWnd;

	ComPtr<IDXGIFactory4>				m_pFactory;
	ComPtr<ID3D12Device>                m_pDevice;
	ComPtr<ID3D12CommandQueue>          m_pQueue;
	ComPtr<IDXGISwapChain3>             m_pSwapChain;
	ComPtr<ID3D12Resource>              m_pColorBuffer[Constants::FrameCount];
	ComPtr<ID3D12CommandAllocator>      m_pCmdAllocator[Constants::FrameCount];
	ComPtr<ID3D12GraphicsCommandList>   m_pCmdList;
	ComPtr<ID3D12DescriptorHeap>        m_pHeapRTV;
	ComPtr<ID3D12Fence>                 m_pFence;
	HANDLE                              m_FenceEvent;
	uint64_t                            m_FenceCounter[Constants::FrameCount];
	uint32_t                            m_FrameIndex;
	D3D12_CPU_DESCRIPTOR_HANDLE         m_HandleRTV[Constants::FrameCount];

	void InitializeDebug();
	void WaitGpu();
	void Present(uint32_t interval);
};
