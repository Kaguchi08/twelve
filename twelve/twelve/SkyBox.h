#pragma once

#include <d3d12.h>
#include <SimpleMath.h>
#include "DescriptorPool.h"
#include "ConstantBuffer.h"
#include "VertexBuffer.h"
#include "RootSignature.h"

class SkyBox
{
public:
	SkyBox();
	~SkyBox();

	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPoolRes,
		DXGI_FORMAT colorFormat,
		DXGI_FORMAT depthFormat);

	void Term();

	void Draw(
		ID3D12GraphicsCommandList* pCmd,
		D3D12_GPU_DESCRIPTOR_HANDLE handle,
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& proj,
		float boxSize);

private:
	DescriptorPool* m_pPoolRes;
	RootSignature m_RootSignature;
	ComPtr<ID3D12PipelineState> m_pPSO;
	ConstantBuffer m_CB[2];
	VertexBuffer m_VB;
	int m_Index;
};
