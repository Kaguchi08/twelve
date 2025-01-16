#pragma once

#define NOMINMAX
#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h> 
#include <DirectXMath.h>

#include "ComPtr.h"
#include "Constants.h"
#include "DescriptorPool.h"
#include "ColorTarget.h"
#include "DepthTarget.h"
#include "CommandList.h"
#include "Fence.h"
#include "Mesh.h"
#include "ConstantBuffer.h"
#include "Texture.h"
#include "Material.h"
#include "RootSignature.h"
#include "InlineUtil.h"
#include "SphereMapConverter.h"
#include "IBLBaker.h"

struct InputState;

class D3D12Wrapper
{
public:
	D3D12Wrapper();
	~D3D12Wrapper();

	bool Initialize(HWND hWind);
	void Terminate();
	void Render();

	void ProcessInput(const InputState& state);

	bool InitializeGraphicsPipeline();
	void ReleaseGraphicsResources();

	void CheckSupportHDR();

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
	ComPtr<IDXGISwapChain4>             m_pSwapChain;
	ColorTarget						    m_RenderTarget[Constants::FrameCount];
	DepthTarget							m_DepthTarget;
	DescriptorPool* m_pPool[POOL_COUNT];
	CommandList							m_CommandList;
	Fence								m_Fence;
	uint32_t                            m_FrameIndex;
	D3D12_VIEWPORT						m_Viewport;
	D3D12_RECT							m_Scissor;
	DXGI_FORMAT							m_BackBufferFormat;


	ComPtr<ID3D12PipelineState>         m_pScenePSO;
	RootSignature                       m_SceneRootSignature;
	ComPtr<ID3D12PipelineState>         m_pTonemapPSO;
	RootSignature                       m_TonemapRootSignature;
	ColorTarget						    m_SceneColorTarget;
	DepthTarget							m_SceneDepthTarget;
	VertexBuffer					    m_QuadVB;
	VertexBuffer                        m_WallVB;
	VertexBuffer	                    m_FloorVB;
	ConstantBuffer                      m_TonemapCB[Constants::FrameCount];
	ConstantBuffer					    m_DirectionalLightCB[Constants::FrameCount];
	ConstantBuffer                      m_LightCB[Constants::FrameCount];
	ConstantBuffer                      m_CameraCB[Constants::FrameCount];
	ConstantBuffer                      m_TransformCB[Constants::FrameCount];
	ConstantBuffer                      m_MeshCB[Constants::FrameCount];
	std::vector<Mesh*>					m_pMeshes;
	Material							m_Material;

	float								m_RotateAngle;

	bool								m_SupportHDR;			// HDRディスプレイをサポートしているかどうか
	float								m_MaxDisplayLuminance;	// ディスプレイの最大輝度値
	float								m_MinDisplayLuminance;	// ディスプレイの裁量輝度値
	int									m_TonemapType;			// トーンマップタイプ
	int									m_ColorSpace;			// 出力色空間
	float								m_BaseLuminance;		// 基準輝度値
	float								m_MaxLuminance;			// 最大輝度値
	float								m_Exposure;				// 露光値
	int                                 m_LightType;            // ライトの種類

	Texture								m_SphereMap;
	SphereMapConverter					m_SphereMapConverter;
	IBLBaker							m_IBLBaker;

	std::chrono::system_clock::time_point m_StartTime;

	void InitializeDebug();
	void Present(uint32_t interval);

	void ChangeDisplayMode(bool hdr);

	void DrawScene(ID3D12GraphicsCommandList* pCmdList);
	void DrawMesh(ID3D12GraphicsCommandList* pCmdList);
	void DrawTonemap(ID3D12GraphicsCommandList* pCmdList);
};
