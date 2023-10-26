#pragma once
#include <DirectXMath.h>

#include <vector>

#include "XMFLOAT_Helper.h"

// ê¸ï™
struct LineSegment {
    LineSegment(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end);

    DirectX::XMFLOAT3 PointOnSegment(float t) const;
    float MinDistSq(const DirectX::XMFLOAT3& point) const;
    static float MinDistSq(const LineSegment& s1, const LineSegment& s2);

    DirectX::XMFLOAT3 start;
    DirectX::XMFLOAT3 end;
};

// ïΩñ 
struct Plane {
    Plane(const DirectX::XMFLOAT3& normal, float d);
    Plane(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, const DirectX::XMFLOAT3& c);

    float SignedDist(const DirectX::XMFLOAT3& point) const;

    DirectX::XMFLOAT3 normal;
    float d;
};

// ãÖ
struct Sphere {
    Sphere(const DirectX::XMFLOAT3& center, float radius);

    bool Contains(const DirectX::XMFLOAT3& point) const;

    DirectX::XMFLOAT3 center;
    float radius;
};

// AABB
struct AABB {
    AABB(const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max);

    void UpdateMinMax(const DirectX::XMFLOAT3& point);
    void Rotate(const DirectX::XMFLOAT3& rotation);
    bool Contains(const DirectX::XMFLOAT3& point) const;
    float MinDistSq(const DirectX::XMFLOAT3& point) const;

    DirectX::XMFLOAT3 min;
    DirectX::XMFLOAT3 max;
};

// OBB
struct OBB {
    DirectX::XMFLOAT3 center;
    DirectX::XMFLOAT3 extend;
    DirectX::XMFLOAT4 rotation;
};

struct Capsule {
    Capsule(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, float radius);

    DirectX::XMFLOAT3 PointOnSegment(float t) const;
    bool Contains(const DirectX::XMFLOAT3& point) const;

    // LineSegment segment;
    float radius;
};

// åç∑îªíË