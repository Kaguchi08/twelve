#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>

using Microsoft::WRL::ComPtr;

class ModelLoader
{
public:
	ModelLoader(class Renderer& renderer);
	~ModelLoader();

	bool LoadPMDModel(const char* filename, struct Model* model);
private:
	class Dx12Wrapper& dx12_;
	class Renderer& renderer_;

	HRESULT CreateMaterialData(struct Model* model);
	HRESULT CreateMaterialAndView(struct Model* model);
};