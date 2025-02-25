//-----------------------------------------------------------------------------
// File : SphereMapConverter.h
// Desc : Sphere Map Converter.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <vector>
#include "DescriptorPool.h"
#include "ConstantBuffer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"


///////////////////////////////////////////////////////////////////////////////
// SphereMapConverter class
///////////////////////////////////////////////////////////////////////////////
class SphereMapConverter
{
	//=========================================================================
	// list of friend classes and methods.
	//=========================================================================
	/* NOTHING */

public:
	//=========================================================================
	// public variables.
	//=========================================================================
	/* NOTHING */

	//=========================================================================
	// public methods.
	//=========================================================================

	//-------------------------------------------------------------------------
	//! @brief      コンストラクタです.
	//-------------------------------------------------------------------------
	SphereMapConverter();

	//-------------------------------------------------------------------------
	//! @brief      デストラクタです.
	//-------------------------------------------------------------------------
	~SphereMapConverter();

	//-------------------------------------------------------------------------
	//! @brief      初期化処理を行います.
	//!
	//! @param[in]      pDevice         デバイスです.
	//! @param[in]      pPoolRTV        レンダーターゲット用ディスクリプタプールです.
	//! @param[in]      pPoolRes        リソース用ディスクリプタプールです.
	//! @param[in]      sphereMapDesc   スフィアマップの構成設定です.
	//! @retval true    初期化に成功.
	//! @retval false   初期化に失敗.
	//-------------------------------------------------------------------------
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPoolRTV,
		DescriptorPool* pPoolRes,
		const D3D12_RESOURCE_DESC& sphereMapDesc,
		int                         mapSize = -1);

	//-------------------------------------------------------------------------
	//! @brief      終了処理を行います.
	//-------------------------------------------------------------------------
	void Term();

	//-------------------------------------------------------------------------
	//! @brief      キューブマップに描画します.
	//!
	//! @param[in]      pCmdList        コマンドリストです
	//! @param[in]      sphereMapHandle スフィアマップのGPUディスクリプタハンドルです.
	//-------------------------------------------------------------------------
	void DrawToCube(
		ID3D12GraphicsCommandList* pCmdList,
		D3D12_GPU_DESCRIPTOR_HANDLE sphereMapHandle);

	//-------------------------------------------------------------------------
	//! @brief      キューブマップの構成設定を取得します.
	//!
	//! @return     キューブマップの構成設定を返却します.
	//-------------------------------------------------------------------------
	D3D12_RESOURCE_DESC GetCubeMapDesc() const;

	//-------------------------------------------------------------------------
	//! @brief      キューブマップのCPUディスクリプタハンドルを取得します.
	//!
	//! @return     キューブマップのCPUディスクリプタハンドルを返却します.
	//-------------------------------------------------------------------------
	D3D12_CPU_DESCRIPTOR_HANDLE GetCubeMapHandleCPU() const;

	//-------------------------------------------------------------------------
	//! @brief      キューブマップのGPUディスクリプタハンドルを取得します.
	//!
	//! @return     キューブマップのGPUディスクリプタハンドルを返却します.
	//-------------------------------------------------------------------------
	D3D12_GPU_DESCRIPTOR_HANDLE GetCubeMapHandleGPU() const;

private:
	//=========================================================================
	// private variables.
	//=========================================================================
	DescriptorPool* m_pPoolRes;         //!< リソース用ディスクリプタプール.
	DescriptorPool* m_pPoolRTV;         //!< レンダーターゲット用ディスクリプタプール.
	ComPtr<ID3D12RootSignature>     m_pRootSig;         //!< ルートシグニチャです.
	ComPtr<ID3D12PipelineState>     m_pPSO;             //!< パイプラインステートです.
	ComPtr<ID3D12Resource>          m_pCubeTex;         //!< キューブマップテクスチャです.
	DescriptorHandle* m_pCubeSRV;         //!< シェーダリソースビューです.
	std::vector<DescriptorHandle*>  m_pCubeRTV;         //!< レンダーターゲットビューです.
	ConstantBuffer                  m_TransformCB[6];   //!< 変換バッファです.
	uint32_t                        m_MipCount;         //!< ミップレベル数です.
	VertexBuffer                    m_VB;               //!< 頂点バッファです.
	IndexBuffer                     m_IB;               //!< インデックスバッファです.

	//=========================================================================
	// private methods.
	//=========================================================================
	void DrawSphere(ID3D12GraphicsCommandList* pCmd);
};
