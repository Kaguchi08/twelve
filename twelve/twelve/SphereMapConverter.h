#pragma once

#include <vector>

#include "DescriptorPool.h"
#include "ConstantBuffer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "RootSignature.h"
#include "Texture.h"

class SphereMapConverter
{
public:
	SphereMapConverter();
	~SphereMapConverter();

	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPoolRTV,
		DescriptorPool* pPoolRes,
		const D3D12_RESOURCE_DESC& sphereMapDesc,
		int mapSize = -1);

	void Term();

	void DrawToCube(
		ID3D12GraphicsCommandList* pCmd,
		D3D12_GPU_DESCRIPTOR_HANDLE sphereMapHandle);

	D3D12_RESOURCE_DESC GetCubeMapDesc() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCubeMapHandleCPU() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetCubeMapHandleGPU() const;

private:
	DescriptorPool* m_pPoolRes;
	DescriptorPool* m_pPoolRTV;
	RootSignature m_RootSignature;
	ComPtr<ID3D12PipelineState> m_pPSO;
	std::unique_ptr<Texture> m_pCubeTex;
	std::vector<DescriptorHandle*> m_pCubeRTVs;
	ConstantBuffer m_TransformCB[6];
	uint32_t m_MipCount;
	VertexBuffer m_VB;
	IndexBuffer m_IB;

	void DrawSphere(ID3D12GraphicsCommandList* pCmd);
};
