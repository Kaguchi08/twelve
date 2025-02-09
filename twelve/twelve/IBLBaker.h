#pragma once

#include "VertexBuffer.h"
#include "ConstantBuffer.h"
#include "DescriptorPool.h"
#include "RootSignature.h"
#include "Texture.h"
#include "ColorTarget.h"

class IBLBaker
{
public:
	static const int    DFGTextureSize = 512;
	static const int    LDTextureSize = 256;
	static const int    MipCount = 8;

	IBLBaker();
	~IBLBaker();

	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPoolRes,
		DescriptorPool* pPoolRTV);

	void Term();

	void IntegrateDFG(ID3D12GraphicsCommandList* pCmd);

	void IntegrateLD(
		ID3D12GraphicsCommandList* pCmd,
		uint32_t mapSize,
		uint32_t mipCount,
		D3D12_GPU_DESCRIPTOR_HANDLE handle);

	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU_DFG() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU_DiffuseLD() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU_SpecularLD() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU_DFG() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU_DiffuseLD() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU_SpecularLD() const;

private:
	ConstantBuffer m_BakeCB[MipCount * 6];
	VertexBuffer m_QuadVB;
	ComPtr<ID3D12Resource> m_pTexDFG;
	ComPtr<ID3D12Resource> m_pTexDiffuseLD;
	ComPtr<ID3D12Resource> m_pTexSpecularLD;
	DescriptorPool* m_pPoolRes;
	DescriptorPool* m_pPoolRTV;
	DescriptorHandle* m_pHandleRTV_DFG;
	DescriptorHandle* m_pHandleRTV_DiffuseLD[6];
	DescriptorHandle* m_pHandleRTV_SpecularLD[MipCount * 6];
	DescriptorHandle* m_pHandleSRV_DFG;
	DescriptorHandle* m_pHandleSRV_DiffuseLD;
	DescriptorHandle* m_pHandleSRV_SpecularLD;
	ComPtr<ID3D12PipelineState> m_pDFG_PSO;
	ComPtr<ID3D12PipelineState> m_pDiffuseLD_PSO;
	ComPtr<ID3D12PipelineState> m_pSpecularLD_PSO;
	RootSignature m_DFG_RootSignature;
	RootSignature m_LD_RootSignature;

	void IntegrateDiffuseLD(
		ID3D12GraphicsCommandList* pCmd,
		D3D12_GPU_DESCRIPTOR_HANDLE handle);

	void IntegrateSpecularLD(
		ID3D12GraphicsCommandList* pCmd,
		D3D12_GPU_DESCRIPTOR_HANDLE handle);
};
