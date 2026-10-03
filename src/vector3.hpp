#ifndef VECTOR3_HPP
#define VECTOR3_HPP

struct Vector3 {
    union {
        struct {
            float x, y, z;
        };
        float v[3];
    };

    Vector3 operator+(const Vector3& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    Vector3 operator-(const Vector3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    Vector3 operator*(const float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
};

namespace VECTOR3 {
// Assuming right handed
inline constexpr Vector3 UP = {0.0f, 1.0f, 0.0f};
inline constexpr Vector3 DOWN = {0.0f, -1.0f, 0.0f};
inline constexpr Vector3 RIGHT = {1.0f, 0.0f, 0.0f};
inline constexpr Vector3 LEFT = {-1.0f, 0.0f, 0.0f};
inline constexpr Vector3 BACK = {0.0f, 0.0f, 1.0f};
inline constexpr Vector3 FRONT = {0.0f, 0.0f, -1.0f};
inline constexpr Vector3 ONES = {1.0f, 1.0f, 1.0f};
inline constexpr Vector3 ZEROS = {0.0f, 0.0f, 0.0f};
} // namespace VECTOR3

#endif // VECTOR3_HPP
