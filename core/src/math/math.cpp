#include "math/math.hpp"
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

// =============================================================================
// Quatérnions
// =============================================================================

float Yume::Math::length(const Quaternion& q) {
    return std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

Quaternion Yume::Math::normalize(const Quaternion& q) {
    float n = Yume::Math::length(q);
    if (n == 0.0f)
        return QUATERNION::IDENTITY;
    float inv = 1.0f / n;
    return {q.x * inv, q.y * inv, q.z * inv, q.w * inv};
}

Quaternion Yume::Math::conjugate(const Quaternion& q) { return {-q.x, -q.y, -q.z, q.w}; }

float Yume::Math::dot(const Quaternion& a, const Quaternion& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

Quaternion Yume::Math::quatFromAxisAngle(const Vector3& axisIn, float angleRadians) {
    Vector3 axis = Yume::Math::normalize(axisIn);
    float half = angleRadians * 0.5f;
    float s = std::sin(half);
    return {axis.x * s, axis.y * s, axis.z * s, std::cos(half)};
}

Quaternion Yume::Math::quatFromEuler(const Vector3& eulerDegrees) {
    // Rotações por eixo, compostas na ordem Rx·Ry·Rz (igual ao Transform).
    // q = qx * qy * qz, pois o produto aplica o fator à direita primeiro.
    Quaternion qx =
        Yume::Math::quatFromAxisAngle({1.0f, 0.0f, 0.0f}, Yume::Math::radians(eulerDegrees.x));
    Quaternion qy =
        Yume::Math::quatFromAxisAngle({0.0f, 1.0f, 0.0f}, Yume::Math::radians(eulerDegrees.y));
    Quaternion qz =
        Yume::Math::quatFromAxisAngle({0.0f, 0.0f, 1.0f}, Yume::Math::radians(eulerDegrees.z));
    return qx * qy * qz;
}

Vector3 Yume::Math::quatToEuler(const Quaternion& qIn) {
    // Inversa da composição Rx·Ry·Rz. Deriva os ângulos a partir da matriz de
    // rotação correspondente (convenção equivalente a extrair de R = Rx·Ry·Rz).
    const Quaternion q = Yume::Math::normalize(qIn);
    const float x = q.x, y = q.y, z = q.z, w = q.w;

    // Elementos relevantes da matriz de rotação (column-major m[c][r]).
    // Usamos a mesma montagem de quatToMatrix3.
    float m02 = 2.0f * (x * z + w * y);        // linha 0, coluna 2
    float m12 = 2.0f * (y * z - w * x);        // linha 1, coluna 2
    float m22 = 1.0f - 2.0f * (x * x + y * y); // linha 2, coluna 2
    float m01 = 2.0f * (x * y - w * z);        // linha 0, coluna 1
    float m00 = 1.0f - 2.0f * (y * y + z * z); // linha 0, coluna 0

    Vector3 euler;
    // pitch (X): -asin(m12), com clamp para evitar NaN perto dos polos.
    float sinPitch = -m12;
    sinPitch = sinPitch < -1.0f ? -1.0f : (sinPitch > 1.0f ? 1.0f : sinPitch);
    euler.x = std::asin(sinPitch);

    const float cosPitch = std::cos(euler.x);
    if (std::fabs(cosPitch) > 1e-4f) {
        euler.y = std::atan2(m02, m22); // yaw (Y)
        euler.z = std::atan2(m01, m00); // roll (Z)
    } else {
        // Gimbal lock: fixa o roll e resolve o yaw.
        euler.y = std::atan2(-2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z));
        euler.z = 0.0f;
    }

    const float toDeg = 180.0f / Yume::Math::PI;
    return {euler.x * toDeg, euler.y * toDeg, euler.z * toDeg};
}

Matrix3 Yume::Math::quatToMatrix3(const Quaternion& qIn) {
    const Quaternion q = Yume::Math::normalize(qIn);
    const float x = q.x, y = q.y, z = q.z, w = q.w;
    const float xx = x * x, yy = y * y, zz = z * z;
    const float xy = x * y, xz = x * z, yz = y * z;
    const float wx = w * x, wy = w * y, wz = w * z;

    // Column-major: cols[c].v[r] é (linha r, coluna c). Mesma orientação que
    // Yume::Math::rotate produz, para o getModelMatrix sair idêntico.
    Matrix3 m(1.0f);
    m[0] = {1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy)};
    m[1] = {2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx)};
    m[2] = {2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy)};
    return m;
}

Matrix4 Yume::Math::quatToMatrix4(const Quaternion& q) {
    return Matrix4(Yume::Math::quatToMatrix3(q));
}

Vector3 Yume::Math::rotateVector(const Quaternion& q, const Vector3& v) {
    // v' = q * (0,v) * conj(q). Forma otimizada (sem construir quatérnion puro).
    Vector3 u{q.x, q.y, q.z};
    float s = q.w;
    Vector3 uv = Yume::Math::cross(u, v);
    Vector3 uuv = Yume::Math::cross(u, uv);
    return v + (uv * (2.0f * s)) + (uuv * 2.0f);
}

Quaternion Yume::Math::integrate(const Quaternion& q, const Vector3& angularVelocity, float dt) {
    // dq/dt = 0.5 * ω_quat * q, com ω_quat = (ωx, ωy, ωz, 0).
    Quaternion omega{angularVelocity.x, angularVelocity.y, angularVelocity.z, 0.0f};
    Quaternion dq = omega * q * (0.5f * dt);
    return Yume::Math::normalize(q + dq);
}
