#include "Collision.h"

LineSegment::LineSegment(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end)
{
}

DirectX::XMFLOAT3 LineSegment::PointOnSegment(float t) const
{
	return DirectX::XMFLOAT3();
}

float LineSegment::MinDistSq(const DirectX::XMFLOAT3& point) const
{
	return 0.0f;
}

float LineSegment::MinDistSq(const LineSegment& s1, const LineSegment& s2)
{
	return 0.0f;
}

Plane::Plane(const DirectX::XMFLOAT3& normal, float d) :
	normal(normal),
	d(d)
{
}

Plane::Plane(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, const DirectX::XMFLOAT3& c)
{
}

float Plane::SignedDist(const DirectX::XMFLOAT3& point) const
{
	return 0.0f;
}

Sphere::Sphere(const DirectX::XMFLOAT3& center, float radius)
{
}

bool Sphere::Contains(const DirectX::XMFLOAT3& point) const
{
	return false;
}

AABB::AABB(const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max)
{
}

void AABB::UpdateMinMax(const DirectX::XMFLOAT3& point)
{
}

void AABB::Rotate(const DirectX::XMFLOAT3& rotation)
{
}

bool AABB::Contains(const DirectX::XMFLOAT3& point) const
{
	return false;
}

float AABB::MinDistSq(const DirectX::XMFLOAT3& point) const
{
	return 0.0f;
}

Capsule::Capsule(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, float radius)
{
}

DirectX::XMFLOAT3 Capsule::PointOnSegment(float t) const
{
	return DirectX::XMFLOAT3();
}

bool Capsule::Contains(const DirectX::XMFLOAT3& point) const
{
	return false;
}
