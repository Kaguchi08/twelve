#pragma once

#include <DirectXMath.h>
#include <d3d12.h>

#include <string>
#include <vector>

class Helper {
   public:
    Helper();
    ~Helper();
};

/// <summary>
/// ���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X���擾����
/// </summary>
/// <param name="modelPath"></param>
/// <param name="texPath"></param>
/// <returns></returns>
extern std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* tex_path);

/// <summary>
/// string(�}���`�o�C�g������)����wstring(���C�h������)���擾����
/// </summary>
/// <param name="str"></param>
/// <returns></returns>
extern std::wstring GetWideStringFromString(const std::string& str);

/// <summary>
/// �t�@�C��������g���q���擾����
/// </summary>
/// <returns></returns>
extern std::string GetExtension(const std::string& path);

/// <summary>
/// �t�@�C��������g���q���擾����(���C�h������)
/// </summary>
/// <returns></returns>
extern std::wstring GetExtensionW(const std::wstring& path);

/// <summary>
/// �e�N�X�`���̃p�X���Z�p���[�^������ŕ�������
/// </summary>
/// <returns></returns>
extern std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');

/// <summary>
/// �f�o�b�N���C���[��L���ɂ���
/// </summary>
extern void EnableDebugLayer();

/// <summary>
/// z�������̕����Ɍ�����s���Ԃ�
/// </summary>
/// <param name="lookat">���������������x�N�g��</param>
/// <param name="up">��x�N�g��</param>
/// <param name="right">�E�x�N�g��</param>
/// <returns></returns>
extern DirectX::XMMATRIX LookAtMatrix(const DirectX::XMVECTOR& lookat, const DirectX::XMFLOAT3& up, const DirectX::XMFLOAT3& right);

/// <summary>
/// ����̃x�N�g�������̕����Ɍ�����s���Ԃ�
/// </summary>
/// <param name="origin">����̃x�N�g��</param>
/// <param name="lookat">���������������x�N�g��</param>
/// <param name="up">��x�N�g��</param>
/// <param name="right">�E�x�N�g��</param>
/// <returns></returns>
extern DirectX::XMMATRIX LookAtMatrix(const DirectX::XMVECTOR& origin, const DirectX::XMVECTOR& lookat, const DirectX::XMFLOAT3& up, const DirectX::XMFLOAT3& right);

/// <summary>
/// �A���C�����g���l��Ԃ�
/// </summary>
/// <param name="size"></param>
/// <param name="alignment"></param>
/// <returns></returns>
extern unsigned int(AligmentedValue(unsigned int size, unsigned int alignment = 16));

/// <summary>
/// �K�E�X�E�F�C�g�l��Ԃ�
/// </summary>
/// <param name="count"></param>
/// <param name="s"></param>
/// <returns></returns>
extern std::vector<float> GetGaussianWeights(size_t count, float s);

float GetYFromXOnBezier(float x, const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b, uint8_t n = 12);

extern void Split(char split_char, char* buffer, std::vector<std::string>& out);

extern void Replace(char search_char, char replace_char, char* buffer);