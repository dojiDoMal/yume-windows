#ifndef YUME_MATH_HPP
#define YUME_MATH_HPP

#include "matrix4.hpp"
#include "vector3.hpp"

namespace Yume {
class Math {
  public:
    static constexpr float PI = 3.14159265358979323846f;
    static float dot(const Vector3& a, const Vector3& b);
    static Vector3 cross(const Vector3& a, const Vector3& b);
    static float length(const Vector3& a);
    static Vector3 normalize(const Vector3& a);
    static float lengthSquared(const Vector3& a);
    static Vector3 min(const Vector3& a, const Vector3& b);
    static Vector3 max(const Vector3& a, const Vector3& b);
    static float radians(float degrees);

    static Matrix4 translate(const Matrix4& m, const Vector3& v);
    static Matrix4 rotate(const Matrix4& m, float angleRadians, const Vector3& axis);
    static Matrix4 scale(const Matrix4& m, const Vector3& v);
    static Matrix4 lookAt(const Vector3& eye, const Vector3& center, const Vector3& up);
    static Matrix4 perspective(float fovyRadians, float aspect, float zNear, float zFar);
    static Matrix4 perspectiveRH_ZO(float fovyRadians, float aspect, float zNear, float zFar);
    static Matrix4 ortho(float left, float right, float bottom, float top, float zNear, float zFar);
    static Matrix4 ortho(float left, float right, float bottom, float top);
    static Matrix4 orthoRH_ZO(float left, float right, float bottom, float top, float zNear,
                              float zFar);
};
} // namespace Yume

#endif // YUME_MATH_HPP