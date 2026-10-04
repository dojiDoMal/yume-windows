#ifndef MATRIX3_HPP
#define MATRIX3_HPP

#include "math/vector3.hpp"

/**
 * @brief Matriz 3x3 de floats, em layout column-major compatível com glm::mat3.
 *
 * Guarda três colunas (@ref cols), cada uma um Vector3. Usada sobretudo para a
 * parte de rotação/escala (sem translação) de uma transformação, por exemplo ao
 * levar normais para o espaço de mundo.
 */
struct Matrix3 {
    Vector3 cols[3]; ///< As três colunas da matriz; cols[c].v[r] é o elemento (linha r, coluna c).

    Matrix3() = default;

    /**
     * @brief Constrói uma matriz diagonal.
     * @param diagonal Valor colocado na diagonal principal (use 1.0f para a identidade).
     */
    explicit constexpr Matrix3(float diagonal)
        : cols{{diagonal, 0.0f, 0.0f}, {0.0f, diagonal, 0.0f}, {0.0f, 0.0f, diagonal}} {}

    /** @brief Constrói a matriz a partir de suas três colunas. */
    constexpr Matrix3(const Vector3& c0, const Vector3& c1, const Vector3& c2) : cols{c0, c1, c2} {}

    /** @brief Acessa a coluna @p col (0..2). */
    constexpr Vector3& operator[](int col) { return cols[col]; }
    /** @brief Acessa a coluna @p col (0..2), somente leitura. */
    constexpr const Vector3& operator[](int col) const { return cols[col]; }

    /** @brief Ponteiro para os 9 floats contíguos (column-major), estilo glm::value_ptr. */
    float* data() { return cols[0].v; }
    /** @brief Versão const de data(). */
    const float* data() const { return cols[0].v; }

    /** @brief Multiplicação matriz * matriz. */
    Matrix3 operator*(const Matrix3& other) const {
        Matrix3 result(0.0f);
        for (int j = 0; j < 3; ++j) {
            const Vector3& b = other.cols[j];
            for (int i = 0; i < 3; ++i) {
                result.cols[j].v[i] =
                    cols[0].v[i] * b.v[0] + cols[1].v[i] * b.v[1] + cols[2].v[i] * b.v[2];
            }
        }
        return result;
    }

    /** @brief Multiplicação matriz * vetor-coluna (M*v). */
    Vector3 operator*(const Vector3& v) const {
        Vector3 result{0.0f, 0.0f, 0.0f};
        for (int i = 0; i < 3; ++i) {
            result.v[i] = cols[0].v[i] * v.x + cols[1].v[i] * v.y + cols[2].v[i] * v.z;
        }
        return result;
    }
};

static_assert(sizeof(Matrix3) == 9 * sizeof(float),
              "Matrix3 must be 9 tightly-packed floats to stay glm::mat3 compatible");

/// @brief Colunas e matrizes constantes úteis de Matrix3.
namespace MATRIX3 {
inline constexpr Vector3 col0 = {1.0f, 0.0f, 0.0f};
inline constexpr Vector3 col1 = {0.0f, 1.0f, 0.0f};
inline constexpr Vector3 col2 = {0.0f, 0.0f, 1.0f};
inline constexpr Matrix3 IDENTITY = {col0, col1, col2};                      ///< Matriz identidade.
inline constexpr Matrix3 ZEROS = {VECTOR3::ZEROS, VECTOR3::ZEROS, VECTOR3::ZEROS}; ///< Matriz nula.
} // namespace MATRIX3

#endif // MATRIX3_HPP
