#include "math.hpp"
#include <cmath>

float Yume::Math::dot(const Vector3& a, const Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 Yume::Math::cross(const Vector3& a, const Vector3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
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

Matrix4 Yume::Math::translate(const Matrix4& m, const Vector3& v) {
    Matrix4 result = m;
    result[3] = m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3];
    return result;
}

Matrix4 Yume::Math::rotate(const Matrix4& m, float angleRadians, const Vector3& axisIn) {
    const float a = angleRadians;
    const float c = std::cos(a);
    const float s = std::sin(a);

    const Vector3 axis = Yume::Math::normalize(axisIn);
    const Vector3 temp = axis * (1.0f - c);

    Matrix4 Rotate(0.0f);
    Rotate[0][0] = c + temp.x * axis.x;
    Rotate[0][1] = temp.x * axis.y + s * axis.z;
    Rotate[0][2] = temp.x * axis.z - s * axis.y;

    Rotate[1][0] = temp.y * axis.x - s * axis.z;
    Rotate[1][1] = c + temp.y * axis.y;
    Rotate[1][2] = temp.y * axis.z + s * axis.x;

    Rotate[2][0] = temp.z * axis.x + s * axis.y;
    Rotate[2][1] = temp.z * axis.y - s * axis.x;
    Rotate[2][2] = c + temp.z * axis.z;

    Matrix4 result(0.0f);
    result[0] = m[0] * Rotate[0][0] + m[1] * Rotate[0][1] + m[2] * Rotate[0][2];
    result[1] = m[0] * Rotate[1][0] + m[1] * Rotate[1][1] + m[2] * Rotate[1][2];
    result[2] = m[0] * Rotate[2][0] + m[1] * Rotate[2][1] + m[2] * Rotate[2][2];
    result[3] = m[3];
    return result;
}

Matrix4 Yume::Math::scale(const Matrix4& m, const Vector3& v) {
    Matrix4 result(0.0f);
    result[0] = m[0] * v.x;
    result[1] = m[1] * v.y;
    result[2] = m[2] * v.z;
    result[3] = m[3];
    return result;
}

Matrix4 Yume::Math::lookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
    const Vector3 f = Yume::Math::normalize(center - eye);
    const Vector3 s = Yume::Math::normalize(Yume::Math::cross(f, up));
    const Vector3 u = Yume::Math::cross(s, f);

    Matrix4 result(1.0f);
    result[0][0] = s.x;
    result[1][0] = s.y;
    result[2][0] = s.z;
    result[0][1] = u.x;
    result[1][1] = u.y;
    result[2][1] = u.z;
    result[0][2] = -f.x;
    result[1][2] = -f.y;
    result[2][2] = -f.z;
    result[3][0] = -Yume::Math::dot(s, eye);
    result[3][1] = -Yume::Math::dot(u, eye);
    result[3][2] = Yume::Math::dot(f, eye);
    return result;
}

Matrix4 Yume::Math::perspective(float fovyRadians, float aspect, float zNear, float zFar) {
    const float tanHalfFovy = std::tan(fovyRadians / 2.0f);

    Matrix4 result(0.0f);
    result[0][0] = 1.0f / (aspect * tanHalfFovy);
    result[1][1] = 1.0f / (tanHalfFovy);
    result[2][2] = -(zFar + zNear) / (zFar - zNear);
    result[2][3] = -1.0f;
    result[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
    return result;
}

Matrix4 Yume::Math::perspectiveRH_ZO(float fovyRadians, float aspect, float zNear, float zFar) {
    const float tanHalfFovy = std::tan(fovyRadians / 2.0f);

    Matrix4 result(0.0f);
    result[0][0] = 1.0f / (aspect * tanHalfFovy);
    result[1][1] = 1.0f / (tanHalfFovy);
    result[2][2] = zFar / (zNear - zFar);
    result[2][3] = -1.0f;
    result[3][2] = -(zFar * zNear) / (zFar - zNear);
    return result;
}

Matrix4 Yume::Math::ortho(float left, float right, float bottom, float top, float zNear,
                          float zFar) {
    Matrix4 result(1.0f);
    result[0][0] = 2.0f / (right - left);
    result[1][1] = 2.0f / (top - bottom);
    result[2][2] = -2.0f / (zFar - zNear);
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    result[3][2] = -(zFar + zNear) / (zFar - zNear);
    return result;
}

Matrix4 Yume::Math::ortho(float left, float right, float bottom, float top) {
    Matrix4 result(1.0f);
    result[0][0] = 2.0f / (right - left);
    result[1][1] = 2.0f / (top - bottom);
    result[2][2] = -1.0f;
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    return result;
}

Matrix4 Yume::Math::orthoRH_ZO(float left, float right, float bottom, float top, float zNear,
                               float zFar) {
    Matrix4 result(1.0f);
    result[0][0] = 2.0f / (right - left);
    result[1][1] = 2.0f / (top - bottom);
    result[2][2] = -1.0f / (zFar - zNear);
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    result[3][2] = -zNear / (zFar - zNear);
    return result;
}