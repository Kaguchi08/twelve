#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <memory>

using Microsoft::WRL::ComPtr;

class Dx12Wrapper;
class PMDActor;

class PMDRenderer
{
	friend PMDActor;

public:
	PMDRenderer(Dx12Wrapper& dx12);
	~PMDRenderer();

	void Initialize();

	void BeforeDraw();

	ID3D12PipelineState* GetPipelineState() {
		return pipeline_state_.Get();
	}
	ID3D12RootSignature* GetRootSignature() {
		return root_signature_.Get();
	}
private:
	Dx12Wrapper& dx12_;
	ComPtr<ID3D12PipelineState> pipeline_state_ = nullptr;
	ComPtr<ID3D12RootSignature> root_signature_ = nullptr;

	std::vector<std::shared_ptr<PMDActor>> actors_;

	// PMD共通テクスチャ
	ComPtr<ID3D12Resource> mWhiteTexture = nullptr;
	ComPtr<ID3D12Resource> mBlackTexture = nullptr;
	ComPtr<ID3D12Resource> mGradTexture = nullptr;

	ID3D12Resource* CreateDefaultTexture(size_t width, size_t height);
	ID3D12Resource* CreateWhiteTexture();
	ID3D12Resource* CreateBlackTexture();
	ID3D12Resource* CreateGradTexture();

	// パイプライン初期化
	HRESULT CreateGraphicsPipeline();
	// ルートシグネチャ初期化
	HRESULT CreateRootSignature();

	bool CheckShaderCompileResult(HRESULT result, ID3DBlob* error=nullptr);
};