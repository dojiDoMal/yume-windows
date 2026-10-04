#ifndef MATRIX4_HPP
#define MATRIX4_HPP

#include "math/matrix3.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"

/**
 * @brief Matriz 4x4 de floats, em layout column-major compatível com glm::mat4.
 *
 * É o tipo central das transformações do engine (model, view, projection).
 * @c cols[c] é a coluna @c c (um Vector4), logo @c m[c][r] é o elemento na
 * coluna @c c, linha @c r. Os 16 floats são contíguos, então data() pode ser
 * passado direto para APIs gráficas, como glm::value_ptr.
 *
 * @see Yume::Math para funções que constroem matrizes de transformação.
 */
struct Matrix4 {
    Vector4 cols[4]; ///< As quatro colunas; cols[c].v[r] é o elemento (linha r, coluna c).

    Matrix4() = default;

    /**
     * @brief Constrói uma matriz diagonal.
     * @param diagonal Valor da diagonal. @c Matrix4(1.0f) gera a identidade,
     *                 como glm::mat4(1.0f).
     */
    explicit constexpr Matrix4(float diagonal)
        : cols{{diagonal, 0.0f, 0.0f, 0.0f},
               {0.0f, diagonal, 0.0f, 0.0f},
               {0.0f, 0.0f, diagonal, 0.0f},
               {0.0f, 0.0f, 0.0f, diagonal}} {}

    /** @brief Constrói a matriz a partir de suas quatro colunas (estilo glm). */
    constexpr Matrix4(const Vector4& c0, const Vector4& c1, const Vector4& c2, const Vector4& c3)
        : cols{c0, c1, c2, c3} {}

    /**
     * @brief Expande uma 3x3 em 4x4 (como glm::mat4(mat3)).
     *
     * Cada coluna da 3x3 recebe w=0 e a quarta coluna vira (0,0,0,1). Descarta
     * a translação e mantém só rotação/escala — usado, por exemplo, pelo skybox
     * com @c mat4(mat3(view)).
     */
    explicit constexpr Matrix4(const Matrix3& m)
        : cols{{m[0].x, m[0].y, m[0].z, 0.0f},
               {m[1].x, m[1].y, m[1].z, 0.0f},
               {m[2].x, m[2].y, m[2].z, 0.0f},
               {0.0f, 0.0f, 0.0f, 1.0f}} {}

    /**
     * @brief Extrai a submatriz 3x3 superior-esquerda (como glm::mat3(mat4)).
     * @return As três primeiras componentes das três primeiras colunas.
     */
    constexpr Matrix3 toMatrix3() const {
        return Matrix3({cols[0].x, cols[0].y, cols[0].z}, {cols[1].x, cols[1].y, cols[1].z},
                       {cols[2].x, cols[2].y, cols[2].z});
    }

    /** @brief Acessa a coluna @p col (0..3); m[c][r] é (linha r, coluna c). */
    Vector4& operator[](int col) { return cols[col]; }
    /** @brief Acessa a coluna @p col (0..3), somente leitura. */
    const Vector4& operator[](int col) const { return cols[col]; }

    /** @brief Ponteiro para os 16 floats contíguos (column-major), estilo glm::value_ptr. */
    float* data() { return cols[0].v; }
    /** @brief Versão const de data(). */
    const float* data() const { return cols[0].v; }

    /** @brief Multiplicação matriz * matriz. */
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

    /** @brief Multiplicação matriz * vetor-coluna (M*v). */
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

/// @brief Colunas e matrizes constantes úteis de Matrix4.
namespace MATRIX4 {
inline constexpr Vector4 col0 = {1.0f, 0.0f, 0.0f, 0.0f};
inline constexpr Vector4 col1 = {0.0f, 1.0f, 0.0f, 0.0f};
inline constexpr Vector4 col2 = {0.0f, 0.0f, 1.0f, 0.0f};
inline constexpr Vector4 col3 = {0.0f, 0.0f, 0.0f, 1.0f};
inline constexpr Matrix4 IDENTITY = {col0, col1, col2, col3};                 ///< Matriz identidade.
inline constexpr Matrix4 ZEROS = {VECTOR4::ZEROS, VECTOR4::ZEROS, VECTOR4::ZEROS, VECTOR4::ZEROS}; ///< Matriz nula.
} // namespace MATRIX4

#endif // MATRIX4_HPP
