#ifndef MATRIX4_HPP
#define MATRIX4_HPP

#include "matrix3.hpp"
#include "vector3.hpp"
#include "vector4.hpp"

// A 4x4 float matrix laid out to be glm::mat4 compatible
//
// m[c] is column c (a Vector4), so m[c][r] is the
// element at column c, row r.
struct Matrix4 {
    // Four columns. cols[c].v[r] == element (row r, column c).
    Vector4 cols[4];

    Matrix4() = default;

    // Diagonal / scalar constructor: Matrix4(1.0f) builds the identity, just
    // like glm::mat4(1.0f). Also enables brace-init: Matrix4 m{1.0f}.
    explicit constexpr Matrix4(float diagonal)
        : cols{{diagonal, 0.0f, 0.0f, 0.0f},
               {0.0f, diagonal, 0.0f, 0.0f},
               {0.0f, 0.0f, diagonal, 0.0f},
               {0.0f, 0.0f, 0.0f, diagonal}} {}

    // Column-wise constructor (columns, matching glm's mat4(vec4, vec4, ...)).
    constexpr Matrix4(const Vector4& c0, const Vector4& c1, const Vector4& c2, const Vector4& c3)
        : cols{c0, c1, c2, c3} {}

    // Expand a 3x3 into a 4x4, matching glm's mat4(mat3): each 3x3 column gets
    // w=0 and the 4th column becomes (0,0,0,1). This drops any translation and
    // keeps only the rotation/scale part (used by the skybox: mat4(mat3(view))).
    explicit constexpr Matrix4(const Matrix3& m)
        : cols{{m[0].x, m[0].y, m[0].z, 0.0f},
               {m[1].x, m[1].y, m[1].z, 0.0f},
               {m[2].x, m[2].y, m[2].z, 0.0f},
               {0.0f, 0.0f, 0.0f, 1.0f}} {}

    // Extract the upper-left 3x3, matching glm's mat3(mat4): the first three
    // components of the first three columns.
    constexpr Matrix3 toMatrix3() const {
        return Matrix3({cols[0].x, cols[0].y, cols[0].z}, {cols[1].x, cols[1].y, cols[1].z},
                       {cols[2].x, cols[2].y, cols[2].z});
    }

    // Column access, glm-style: m[c] is a column, m[c][r] is (row r, col c).
    Vector4& operator[](int col) { return cols[col]; }
    const Vector4& operator[](int col) const { return cols[col]; }

    // Raw pointer to the 16 contiguous floats (column-major), the equivalent
    // of glm::value_ptr(m).
    float* data() { return cols[0].v; }
    const float* data() const { return cols[0].v; }

    // Matrix * matrix. Result column j = this applied to other's column j.
    Matrix4 operator*(const Matrix4& other) const {
        Matrix4 result(0.0f);
        for (int j = 0; j < 4; ++j) {
            const Vector4& b = other.cols[j];
            for (int i = 0; i < 4; ++i) {
                result.cols[j].v[i] = cols[0].v[i] * b.v[0] + cols[1].v[i] * b.v[1] +
                                      cols[2].v[i] * b.v[2] + cols[3].v[i] * b.v[3];
            }
        }
        return result;
    }

    // Matrix * column vector (v treated as a column, M*v).
    Vector4 operator*(const Vector4& v) const {
        Vector4 result{0.0f, 0.0f, 0.0f, 0.0f};
        for (int i = 0; i < 4; ++i) {
            result.v[i] =
                cols[0].v[i] * v.x + cols[1].v[i] * v.y + cols[2].v[i] * v.z + cols[3].v[i] * v.w;
        }
        return result;
    }
};

static_assert(sizeof(Matrix4) == 16 * sizeof(float),
              "Matrix4 must be 16 tightly-packed floats to stay glm::mat4 compatible");

namespace MATRIX4 {
inline constexpr Vector4 col0 = {1.0f, 0.0f, 0.0f, 0.0f};
inline constexpr Vector4 col1 = {0.0f, 1.0f, 0.0f, 0.0f};
inline constexpr Vector4 col2 = {0.0f, 0.0f, 1.0f, 0.0f};
inline constexpr Vector4 col3 = {0.0f, 0.0f, 0.0f, 1.0f};
inline constexpr Matrix4 IDENTITY = {col0, col1, col2, col3};
inline constexpr Matrix4 ZEROS = {VECTOR4::ZEROS, VECTOR4::ZEROS, VECTOR4::ZEROS, VECTOR4::ZEROS};
} // namespace MATRIX4

#endif // MATRIX4_HPP
