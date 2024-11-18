#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

struct MeshVertex2
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexCoord;
	DirectX::XMFLOAT3 Tangent;

	MeshVertex2() = default;

	MeshVertex2(
		DirectX::XMFLOAT3 const& position,
		DirectX::XMFLOAT3 const& normal,
		DirectX::XMFLOAT2 const& texCoord,
		DirectX::XMFLOAT3 const& tangent)
		:
		Position(position),
		Normal(normal),
		TexCoord(texCoord),
		Tangent(tangent)
	{
	}

	static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:
	static const int InputElementCount = 4;
	static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

struct Material2
{
	DirectX::XMFLOAT3 Diffuse;
	DirectX::XMFLOAT3 Specular;
	float Alpha;
	float Shininess;
	std::string DiffuseMap;
};

struct Mesh2
{
	std::vector<MeshVertex2> Vertices;
	std::vector<uint32_t> Indices;
	uint32_t MaterialId;
};

bool LoadMesh(
	const wchar_t* fileName,
	std::vector<Mesh2>& meshes,
	std::vector<Material2>& materials);
