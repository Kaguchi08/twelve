#include "Mesh.h"

#include "Logger.h"

Mesh::Mesh()
	: m_MaterialId(INT32_MAX)
	, m_IndexCount(0)
{
}

Mesh::~Mesh()
{
	Term();
}

bool Mesh::Init(ID3D12Device* pDevice, const ResMesh& resourse)
{
	if (pDevice == nullptr)
	{
		ELOG("Error : Invalid Argument.");
		return false;
	}

	if (!m_VB.Init<MeshVertex>(pDevice, resourse.Vertices.size(), resourse.Vertices.data()))
	{
		ELOG("Error : VertexBuffer::Init() Failed.");
		return false;
	}

	if (!m_IB.Init(pDevice, resourse.Indices.size(), resourse.Indices.data()))
	{
		ELOG("Error : IndexBuffer::Init() Failed.");
		return false;
	}

	m_MaterialId = resourse.MaterialId;
	m_IndexCount = uint32_t(resourse.Indices.size());

	return true;
}

void Mesh::Term()
{
	m_VB.Term();
	m_IB.Term();
	m_MaterialId = UINT32_MAX;
	m_IndexCount = 0;
}

void Mesh::Draw(ID3D12GraphicsCommandList* pCmdList)
{
	auto VBV = m_VB.GetView();
	auto IBV = m_IB.GetView();

	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCmdList->IASetVertexBuffers(0, 1, &VBV);
	pCmdList->IASetIndexBuffer(&IBV);

	pCmdList->DrawIndexedInstanced(m_IndexCount, 1, 0, 0, 0);
}

uint32_t Mesh::GetMaterialId() const
{
	return m_MaterialId;
}
