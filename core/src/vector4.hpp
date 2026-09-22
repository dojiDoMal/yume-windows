#ifndef VECTOR4_HPP
#define VECTOR4_HPP

struct Vector4 {
    union {
        struct {
            float x, y, z, w;
        };
        float v[4];
    };

    Vector4 operator+(const Vector4& other) const {
        return {x + other.x, y + other.y, z + other.z, w + other.w};
    }

    Vector4 operator-(const Vector4& other) const {
        return {x - other.x, y - other.y, z - other.z, w - other.w};
    }

    Vector4 operator*(const float scalar) const {
        return {x * scalar, y * scalar, z * scalar, w * scalar};
    }
};

namespace VECTOR4 {
inline constexpr Vector4 ONES = {1.0f, 1.0f, 1.0f, 1.0f};
inline constexpr Vector4 ZEROS = {0.0f, 0.0f, 0.0f, 0.0f};
} // namespace VECTOR4

#endif // VECTOR4_HPP
