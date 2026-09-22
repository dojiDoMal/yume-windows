#include "math.hpp"
#include <cmath>

float Yume::Math::dot(const Vector3& a, const Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 Yume::Math::min(const Vector3& a, const Vector3& b) {
    return {a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z};
}

Vector3 Yume::Math::max(const Vector3& a, const Vector3& b) {
    return {a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z};
}

float Yume::Math::length(const Vector3& a) { return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z); }

Vector3 Yume::Math::normalize(const Vector3& a) {
    float norm = Yume::Math::length(a);
    if (norm == 0.0f)
        return VECTOR3::ZEROS;
    return {a.x / norm, a.y / norm, a.z / norm};
}

float Yume::Math::lengthSquared(const Vector3& a) { return (a.x * a.x + a.y * a.y + a.z * a.z); }

float Yume::Math::radians(float degrees) {
    constexpr float factor = Yume::Math::PI / 180.0f;
    return degrees * factor;
}