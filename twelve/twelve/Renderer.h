#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <memory>

using Microsoft::WRL::ComPtr;

class Dx12Wrapper;


class Renderer {
public:
private:
	Dx12Wrapper& dx_;
	// �p�C�v���C��
	ComPtr<ID3D12PipelineState> model_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> pera_pipeline_state_ = nullptr;
	ComPtr<ID3D12PipelineState> pera_pipeline_state_2_ = nullptr;

	// ���[�g�V�O�l�`��
	ComPtr<ID3D12RootSignature> model_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> pera_root_signature_ = nullptr;
	ComPtr<ID3D12RootSignature> pera_root_signature_2_ = nullptr;


};