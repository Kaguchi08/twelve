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
/// モデルのパスとテクスチャのパスから合成パスを取得する
/// </summary>
/// <param name="modelPath"></param>
/// <param name="texPath"></param>
/// <returns></returns>
extern std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* tex_path);

/// <summary>
/// string(マルチバイト文字列)からwstring(ワイド文字列)を取得する
/// </summary>
/// <param name="str"></param>
/// <returns></returns>
extern std::wstring GetWideStringFromString(const std::string& str);

/// <summary>
/// ファイル名から拡張子を取得する
/// </summary>
/// <returns></returns>
extern std::string GetExtension(const std::string& path);

/// <summary>
/// ファイル名から拡張子を取得する(ワイド文字列)
/// </summary>
/// <returns></returns>
extern std::wstring GetExtensionW(const std::wstring& path);

/// <summary>
/// テクスチャのパスをセパレータ文字列で分離する
/// </summary>
/// <returns></returns>
extern std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');

/// <summary>
/// デバックレイヤーを有効にする
/// </summary>
extern void EnableDebugLayer();

/// <summary>
/// z軸を特定の方向に向ける行列を返す
/// </summary>
/// <param name="lookat">向かせたい方向ベクトル</param>
/// <param name="up">上ベクトル</param>
/// <param name="right">右ベクトル</param>
/// <returns></returns>
extern DirectX::XMMATRIX LookAtMatrix(const DirectX::XMVECTOR& lookat, const DirectX::XMFLOAT3& up, const DirectX::XMFLOAT3& right);

/// <summary>
/// 特定のベクトルを特定の方向に向ける行列を返す
/// </summary>
/// <param name="origin">特定のベクトル</param>
/// <param name="lookat">向かせたい方向ベクトル</param>
/// <param name="up">上ベクトル</param>
/// <param name="right">右ベクトル</param>
/// <returns></returns>
extern DirectX::XMMATRIX LookAtMatrix(const DirectX::XMVECTOR& origin, const DirectX::XMVECTOR& lookat, const DirectX::XMFLOAT3& up, const DirectX::XMFLOAT3& right);

/// <summary>
/// アライメント数値を返す
/// </summary>
/// <param name="size"></param>
/// <param name="alignment"></param>
/// <returns></returns>
extern unsigned int(AligmentedValue(unsigned int size, unsigned int alignment = 16));

/// <summary>
/// ガウスウェイト値を返す
/// </summary>
/// <param name="count"></param>
/// <param name="s"></param>
/// <returns></returns>
extern std::vector<float> GetGaussianWeights(size_t count, float s);

float GetYFromXOnBezier(float x, const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b, uint8_t n = 12);

extern void Split(char split_char, char* buffer, std::vector<std::string>& out);

extern void Replace(char search_char, char replace_char, char* buffer);