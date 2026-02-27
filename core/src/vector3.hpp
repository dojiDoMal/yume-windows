#ifndef VECTOR3_HPP
#define VECTOR3_HPP

struct Vector3 {
    union {
        struct {
            float x, y, z;
        };
        float v[3];
    };
};

namespace VECTOR3 {
// Assuming right handed
inline constexpr Vector3 UP = {0.0f, 1.0f, 0.0f};
inline constexpr Vector3 DOWN = {0.0f, -1.0f, 0.0f};
inline constexpr Vector3 RIGHT = {1.0f, 0.0f, 0.0f};
inline constexpr Vector3 LEFT = {-1.0f, 0.0f, 0.0f};
inline constexpr Vector3 BACK = {1.0f, 0.0f, 1.0f};
inline constexpr Vector3 FRONT = {1.0f, 0.0f, -1.0f};
} // namespace VECTOR3

#endif // VECTOR3_HPP
