#pragma once

#include "ResMesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

class Mesh
{
public:
	Mesh();
	virtual ~Mesh();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="pDevice">デバイス</param>
	/// <param name="resourse">リソースメッシュ</param>
	/// <returns></returns>
	bool Init(
		ID3D12Device* pDevice,
		const ResMesh& resourse);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	/// <summary>
	/// 描画処理
	/// </summary>
	/// <param name="pCmdList">コマンドリスト</param>
	void Draw(ID3D12GraphicsCommandList* pCmdList);

	uint32_t GetMaterialId() const;

private:
	VertexBuffer m_VB; // 頂点バッファ
	IndexBuffer m_IB; // インデックスバッファ
	uint32_t m_MaterialId; // マテリアル番号
	uint32_t m_IndexCount; // インデックス数

	Mesh(const Mesh&) = delete;
	void operator=(const Mesh&) = delete;
};
