#include "Helper.h"

Helper::Helper()
{
}

Helper::~Helper()
{
}

std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* tex_path)
{
	auto pathIndex = modelPath.rfind('/');
	auto folderPath = modelPath.substr(0, pathIndex + 1);
	return folderPath + tex_path;
}

std::wstring GetWideStringFromString(const std::string& str)
{
	auto wideSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str.c_str(), -1, nullptr, 0);
	std::wstring wStr;
	wStr.resize(wideSize);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str.c_str(), -1, &wStr[0], wideSize);
	return wStr;
}

std::string GetExtension(const std::string& path)
{
	auto index = path.rfind('.');
	return path.substr(index + 1, path.length() - index - 1);
}

std::wstring GetExtensionW(const std::wstring& path)
{
	auto index = path.rfind(L'.');
	return path.substr(index + 1, path.length() - index - 1);
}

std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter)
{
	auto index = path.find(splitter);
	std::pair<std::string, std::string> result;
	result.first = path.substr(0, index);
	result.second = path.substr(index + 1, path.length() - index - 1);
	return result;
}

void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
	{
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}

DirectX::XMMATRIX LookAtMatrix(const DirectX::XMVECTOR& lookat, const DirectX::XMFLOAT3& up, const DirectX::XMFLOAT3& right)
{
	DirectX::XMVECTOR vz = lookat;
	// ‰¼Ž²
	DirectX::XMVECTOR dummyAxis = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&up));
	DirectX::XMVECTOR vx = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(dummyAxis, vz));
	DirectX::XMVECTOR vy = DirectX::XMVector3Cross(vz, vx);

	if (abs(DirectX::XMVector3Dot(vy, vz).m128_f32[0]) == 1.0f)
	{
		dummyAxis = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&right));
		vy = DirectX::XMVector3Cross(vz, dummyAxis);
		vx = DirectX::XMVector3Cross(vy, vz);
	}

	DirectX::XMMATRIX mat = DirectX::XMMatrixIdentity();
	mat.r[0] = vx;
	mat.r[1] = vy;
	mat.r[2] = vz;

	return mat;
}

DirectX::XMMATRIX LookAtMatrix(const DirectX::XMVECTOR& origin, const DirectX::XMVECTOR& lookat, const DirectX::XMFLOAT3& up, const DirectX::XMFLOAT3& right)
{
	return DirectX::XMMatrixTranspose(LookAtMatrix(origin, up, right)) * LookAtMatrix(lookat, up, right);
}

unsigned int AligmentedValue(unsigned int size, unsigned int alignment)
{
	return (size + alignment - (size % alignment));
}

std::vector<float> GetGaussianWeights(size_t count, float s)
{
	std::vector<float> weights(count);
	float x = 0.0f;
	float total = 0.0f;

	for (auto& weight : weights)
	{
		weight = expf(-x * x / (2.0f * s * s));
		total += weight;
		x += 1.0f;
	}

	total = total * 2.0f - 1.0f;

	for (auto& weight : weights)
	{
		weight /= total;
	}

	return weights;
}
