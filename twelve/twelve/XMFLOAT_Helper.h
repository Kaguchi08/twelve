//-------------------------------------------------------------------------------------------------------------
//参考
// https://qiita.com/HnniTns/items/6e7edc82775a86923cef
//-------------------------------------------------------------------------------------------------------------

#pragma once

#include <DirectXMath.h>
#include <initializer_list>
#include <array>
#include <cmath>
#include <cassert>
#include <limits>

//-------------------------------------------------------------------------------------------------------------
//. XMFLOAT4系
//-------------------------------------------------------------------------------------------------------------

static inline void operator+= (DirectX::XMFLOAT4& v1, const DirectX::XMFLOAT4& v2)
{
    v1.x += v2.x;
    v1.y += v2.y;
    v1.z += v2.z;
    v1.w += v2.w;
}

static inline void operator-= (DirectX::XMFLOAT4& v1, const DirectX::XMFLOAT4& v2)
{
    v1.x -= v2.x;
    v1.y -= v2.y;
    v1.z -= v2.z;
    v1.w -= v2.w;
}

static inline void operator/= (DirectX::XMFLOAT4& v1, const DirectX::XMFLOAT4& v2)
{
    v1.x /= v2.x;
    v1.y /= v2.y;
    v1.z /= v2.z;
    v1.w /= v2.w;
}

static inline void operator*= (DirectX::XMFLOAT4& v1, const DirectX::XMFLOAT4& v2)
{
    v1.x *= v2.x;
    v1.y *= v2.y;
    v1.z *= v2.z;
    v1.w *= v2.w;
}

static inline void operator%= (DirectX::XMFLOAT4& v1, const DirectX::XMFLOAT4& v2)
{
    v1.x = ::fmodf(v1.x, v2.x);
    v1.y = ::fmodf(v1.y, v2.y);
    v1.z = ::fmodf(v1.z, v2.z);
    v1.w = ::fmodf(v1.w, v2.w);
}

_NODISCARD static inline constexpr auto operator+ (const DirectX::XMFLOAT4& v1, const DirectX::XMFLOAT4& v2)
{
    return DirectX::XMFLOAT4{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w };
}

_NODISCARD static inline constexpr auto operator- (const DirectX::XMFLOAT4& v1, const DirectX::XMFLOAT4& v2)
{
    return DirectX::XMFLOAT4{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w };
}

_NODISCARD static inline constexpr auto operator* (const DirectX::XMFLOAT4& v1, const DirectX::XMFLOAT4& v2)
{
    return DirectX::XMFLOAT4{ v1.x* v2.x, v1.y* v2.y, v1.z* v2.z, v1.w* v2.w };
}

_NODISCARD static inline constexpr auto operator/ (const DirectX::XMFLOAT4& v1, const DirectX::XMFLOAT4& v2)
{
    return DirectX::XMFLOAT4{ v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w };
}

_NODISCARD static inline constexpr auto operator% (const DirectX::XMFLOAT4& v1, const DirectX::XMFLOAT4& v2)
{
    return DirectX::XMFLOAT4{ ::fmodf(v1.x, v2.x), ::fmodf(v1.y, v2.y), ::fmodf(v1.z, v2.z), ::fmodf(v1.w, v2.w) };
}

_NODISCARD static inline constexpr auto operator- (const DirectX::XMFLOAT4& v1)
{
    return DirectX::XMFLOAT4{ -v1.x, -v1.y, -v1.z, -v1.w };
}

//-------------------------------------------------------------------------------------------------------------
//. XMFLOAT3系
//-------------------------------------------------------------------------------------------------------------

static inline void operator+= (DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2)
{
    v1.x += v2.x;
    v1.y += v2.y;
    v1.z += v2.z;
}

static inline void operator-= (DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2)
{
    v1.x -= v2.x;
    v1.y -= v2.y;
    v1.z -= v2.z;
}

static inline void operator*= (DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2)
{
    v1.x *= v2.x;
    v1.y *= v2.y;
    v1.z *= v2.z;
}

static inline void operator/= (DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2)
{
    v1.x /= v2.x;
    v1.y /= v2.y;
    v1.z /= v2.z;
}

static inline void operator%= (DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2)
{
    v1.x = ::fmodf(v1.x, v2.x);
    v1.y = ::fmodf(v1.y, v2.y);
    v1.z = ::fmodf(v1.z, v2.z);
}

_NODISCARD static inline constexpr auto operator+ (const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2)
{
    return DirectX::XMFLOAT3{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

_NODISCARD static inline constexpr auto operator- (const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2)
{
    return DirectX::XMFLOAT3{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

_NODISCARD static inline constexpr auto operator* (const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2)
{
    return DirectX::XMFLOAT3{ v1.x* v2.x, v1.y* v2.y, v1.z* v2.z };
}

_NODISCARD static inline constexpr auto operator/ (const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2)
{
    return DirectX::XMFLOAT3{ v1.x / v2.x, v1.y / v2.y, v1.z / v2.z };
}

_NODISCARD static inline constexpr auto operator% (const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2)
{
    return DirectX::XMFLOAT3{ ::fmodf(v1.x, v2.x), ::fmodf(v1.y, v2.y), ::fmodf(v1.z, v2.z) };
}

_NODISCARD static inline constexpr auto operator- (const DirectX::XMFLOAT3& v1)
{
    return DirectX::XMFLOAT3{ -v1.x, -v1.y, -v1.z };
}

//-------------------------------------------------------------------------------------------------------------
//. XMFLOAT2系
//-------------------------------------------------------------------------------------------------------------

static inline void operator+= (DirectX::XMFLOAT2& v1, const DirectX::XMFLOAT2& v2)
{
    v1.x += v2.x;
    v1.y += v2.y;
}

static inline void operator-= (DirectX::XMFLOAT2& v1, const DirectX::XMFLOAT2& v2)
{
    v1.x -= v2.x;
    v1.y -= v2.y;
}

static inline void operator/= (DirectX::XMFLOAT2& v1, const DirectX::XMFLOAT2& v2)
{
    v1.x /= v2.x;
    v1.y /= v2.y;
}

static inline void operator*= (DirectX::XMFLOAT2& v1, const DirectX::XMFLOAT2& v2)
{
    v1.x *= v2.x;
    v1.y *= v2.y;
}

static inline void operator%= (DirectX::XMFLOAT2& v1, const DirectX::XMFLOAT2& v2)
{
    v1.x = ::fmodf(v1.x, v2.x);
    v1.y = ::fmodf(v1.y, v2.y);
}

_NODISCARD static inline constexpr auto operator+ (const DirectX::XMFLOAT2& v1, const DirectX::XMFLOAT2& v2)
{
    return DirectX::XMFLOAT2{ v1.x + v2.x, v1.y + v2.y };
}

_NODISCARD static inline constexpr auto operator- (const DirectX::XMFLOAT2& v1, const DirectX::XMFLOAT2& v2)
{
    return DirectX::XMFLOAT2{ v1.x - v2.x, v1.y - v2.y };
}

_NODISCARD static inline constexpr auto operator* (const DirectX::XMFLOAT2& v1, const DirectX::XMFLOAT2& v2)
{
    return DirectX::XMFLOAT2{ v1.x* v2.x, v1.y* v2.y };
}

_NODISCARD static inline constexpr auto operator/ (const DirectX::XMFLOAT2& v1, const DirectX::XMFLOAT2& v2)
{
    return DirectX::XMFLOAT2{ v1.x / v2.x, v1.y / v2.y };
}

_NODISCARD static inline constexpr auto operator% (const DirectX::XMFLOAT2& v1, const DirectX::XMFLOAT2& v2)
{
    return DirectX::XMFLOAT2{ ::fmodf(v1.x, v2.x), ::fmodf(v1.y, v2.y) };
}

_NODISCARD static inline constexpr auto operator- (const DirectX::XMFLOAT2& v1)
{
    return DirectX::XMFLOAT2{ -v1.x, -v1.y };
}

//-------------------------------------------------------------------------------------------------------------
// 変換関数系
//-------------------------------------------------------------------------------------------------------------

//. XMVECTOR変換-----------------------------------------------------------------------------------------------

_NODISCARD static inline auto ToXMVECTOR(const DirectX::XMFLOAT4& vec)
{
    return DirectX::XMLoadFloat4(&vec);
}

_NODISCARD static inline auto ToXMVECTOR(const DirectX::XMFLOAT3& vec)
{
    return DirectX::XMLoadFloat3(&vec);
}

_NODISCARD static inline auto ToXMVECTOR(const DirectX::XMFLOAT2& vec)
{
    return DirectX::XMLoadFloat2(&vec);
}

_NODISCARD static inline auto ToXMVECTOR(const float vec)
{
    return DirectX::XMLoadFloat(&vec);
}

//. XMVECTOR変換(正規化)---------------------------------------------------------------------------------------

_NODISCARD static inline auto ToNormalizeXMVECTOR(const DirectX::XMFLOAT4& vec)
{
    return DirectX::XMVector4Normalize(DirectX::XMLoadFloat4(&vec));
}

_NODISCARD static inline auto ToNormalizeXMVECTOR(const DirectX::XMFLOAT3& vec)
{
    return DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&vec));
}

_NODISCARD static inline auto ToNormalizeXMVECTOR(const DirectX::XMFLOAT2& vec)
{
    return DirectX::XMVector2Normalize(DirectX::XMLoadFloat2(&vec));
}

//. XMFLOATなどに変換-----------------------------------------------------------------------------------------------

_NODISCARD static inline auto ToXMFLOAT4(const DirectX::XMVECTOR& vec)
{
    DirectX::XMFLOAT4 rv;

    DirectX::XMStoreFloat4(&rv, vec);

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT3(const DirectX::XMVECTOR& vec)
{
    DirectX::XMFLOAT3 rv;

    DirectX::XMStoreFloat3(&rv, vec);

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT2(const DirectX::XMVECTOR& vec)
{
    DirectX::XMFLOAT2 rv;

    DirectX::XMStoreFloat2(&rv, vec);

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT1(const DirectX::XMVECTOR& vec)
{
    float rv;

    DirectX::XMStoreFloat(&rv, vec);

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT(const DirectX::XMINT4& vec)
{
    DirectX::XMFLOAT4 rv{ (float)vec.x, (float)vec.y, (float)vec.z, (float)vec.w };

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT(const DirectX::XMINT3& vec)
{
    DirectX::XMFLOAT3 rv{ (float)vec.x, (float)vec.y, (float)vec.z };

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT(const DirectX::XMINT2& vec)
{
    DirectX::XMFLOAT2 rv{ (float)vec.x, (float)vec.y };

    return rv;
}

_NODISCARD static inline auto ToXMINT(const DirectX::XMFLOAT4& vec)
{
    DirectX::XMINT4 rv{ (int)vec.x, (int)vec.y, (int)vec.z, (int)vec.w };

    return rv;
}

_NODISCARD static inline auto ToXMINT(const DirectX::XMFLOAT3& vec)
{
    DirectX::XMINT3 rv{ (int)vec.x, (int)vec.y, (int)vec.z };

    return rv;
}

_NODISCARD static inline auto ToXMINT(const DirectX::XMFLOAT2& vec)
{
    DirectX::XMINT2 rv{ (int)vec.x, (int)vec.y };

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT(const DirectX::XMUINT4& vec)
{
    DirectX::XMFLOAT4 rv{ (float)vec.x, (float)vec.y, (float)vec.z, (float)vec.w };

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT(const DirectX::XMUINT3& vec)
{
    DirectX::XMFLOAT3 rv{ (float)vec.x, (float)vec.y, (float)vec.z };

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT(const DirectX::XMUINT2& vec)
{
    DirectX::XMFLOAT2 rv{ (float)vec.x, (float)vec.y };

    return rv;
}

_NODISCARD static inline auto ToXMUINT(const DirectX::XMFLOAT4& vec)
{
    DirectX::XMUINT4 rv{ (uint32_t)vec.x, (uint32_t)vec.y, (uint32_t)vec.z, (uint32_t)vec.w };

    return rv;
}

_NODISCARD static inline auto ToXMUINT(const DirectX::XMFLOAT3& vec)
{
    DirectX::XMUINT3 rv{ (uint32_t)vec.x, (uint32_t)vec.y, (uint32_t)vec.z };

    return rv;
}

_NODISCARD static inline auto ToXMUINT(const DirectX::XMFLOAT2& vec)
{
    DirectX::XMUINT2 rv{ (uint32_t)vec.x, (uint32_t)vec.y };

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT(const std::array<float, 4>& vec)
{
    DirectX::XMFLOAT4 rv{ vec.front(), vec.at(1), vec.at(2), vec.back() };

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT(const std::array<float, 3>& vec)
{
    DirectX::XMFLOAT3 rv{ vec.front(), vec.at(1), vec.back() };

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT(const std::array<float, 2>& vec)
{
    DirectX::XMFLOAT2 rv{ vec.front(), vec.back() };

    return rv;
}

_NODISCARD static inline auto ToArray(const DirectX::XMFLOAT4& vec)
{
    std::array<float, 4> rv{ vec.x, vec.y, vec.z, vec.w };

    return rv;
}

_NODISCARD static inline auto ToArray(const DirectX::XMFLOAT3& vec)
{
    std::array<float, 3> rv{ vec.x, vec.y, vec.z };

    return rv;
}

_NODISCARD static inline auto ToArray(const DirectX::XMFLOAT2& vec)
{
    std::array<float, 2> rv{ vec.x, vec.y };

    return rv;
}

//. XMMATRIX変換-----------------------------------------------------------------------------------------

_NODISCARD static inline auto ToXMMATRIX(const DirectX::XMFLOAT4X4& vec)
{
    return DirectX::XMLoadFloat4x4(&vec);
}

_NODISCARD static inline auto ToXMMATRIX(const DirectX::XMFLOAT3X3& vec)
{
    return DirectX::XMLoadFloat3x3(&vec);
}

_NODISCARD static inline auto ToXMMATRIX(const DirectX::XMFLOAT3X4& vec)
{
    return DirectX::XMLoadFloat3x4(&vec);
}

_NODISCARD static inline auto ToXMMATRIX(const DirectX::XMFLOAT4X3& vec)
{
    return DirectX::XMLoadFloat4x3(&vec);
}

//. XMFLOAT変換-----------------------------------------------------------------------------------------------

_NODISCARD static inline auto ToXMFLOAT4X4(const DirectX::XMMATRIX& mat)
{
    DirectX::XMFLOAT4X4 rv;

    DirectX::XMStoreFloat4x4(&rv, mat);

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT3X3(const DirectX::XMMATRIX& mat)
{
    DirectX::XMFLOAT3X3 rv;

    DirectX::XMStoreFloat3x3(&rv, mat);

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT3X4(const DirectX::XMMATRIX& mat)
{
    DirectX::XMFLOAT3X4 rv;

    DirectX::XMStoreFloat3x4(&rv, mat);

    return rv;
}

_NODISCARD static inline auto ToXMFLOAT4X3(const DirectX::XMMATRIX& mat)
{
    DirectX::XMFLOAT4X3 rv;

    DirectX::XMStoreFloat4x3(&rv, mat);

    return rv;
}

//-------------------------------------------------------------------------------------------------------------
// 特殊変換関数
//-------------------------------------------------------------------------------------------------------------

// 型上げ-----------------------------------------------------------------------------------------------

_NODISCARD static inline auto RaiseToXMFLOAT4(const DirectX::XMFLOAT3& vec, const float w_component = 0.f)
{
    return DirectX::XMFLOAT4{ vec.x, vec.y, vec.z, w_component };
}

_NODISCARD static inline auto RaiseToXMFLOAT4(const DirectX::XMFLOAT2& vec, const float z_component = 0.f, const float w_component = 0.f)
{
    return DirectX::XMFLOAT4{ vec.x, vec.y, z_component, w_component };
}

_NODISCARD static inline auto RaiseToXMFLOAT3(const DirectX::XMFLOAT2& vec, const float z_component = 0.f)
{
    return DirectX::XMFLOAT3{ vec.x, vec.y, z_component };
}

// 型下げ-----------------------------------------------------------------------------------------------

// W, Z成分が切り捨てられる
_NODISCARD static inline auto LowerToXMFLOAT2(const DirectX::XMFLOAT4& vec)
{
    return DirectX::XMFLOAT2{ vec.x, vec.y };
}

// Z成分が切り捨てられる
_NODISCARD static inline auto LowerToXMFLOAT2(const DirectX::XMFLOAT3& vec)
{
    return DirectX::XMFLOAT2{ vec.x, vec.y };
}

// W成分が切り捨てられる
_NODISCARD static inline auto LowerToXMFLOAT3(const DirectX::XMFLOAT4& vec)
{
    return DirectX::XMFLOAT3{ vec.x, vec.y, vec.z };
}

