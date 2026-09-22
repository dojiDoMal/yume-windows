#ifndef YUME_MATH_HPP
#define YUME_MATH_HPP

#include "vector3.hpp"

namespace Yume {
class Math {
  public:
    static constexpr float PI = 3.14159265358979323846f;
    static float dot(const Vector3& a, const Vector3& b);
    static float length(const Vector3& a);
    static Vector3 normalize(const Vector3& a);
    static float lengthSquared(const Vector3& a);
    static Vector3 min(const Vector3& a, const Vector3& b);
    static Vector3 max(const Vector3& a, const Vector3& b);
    static float radians(float degrees);
};
} // namespace Yume

#endif // YUME_MATH_HPP