#ifndef VECTOR4_HPP
#define VECTOR4_HPP

/**
 * @brief Vetor de 4 componentes de ponto flutuante (x, y, z, w).
 *
 * Usado para coordenadas homogêneas, cores e colunas de Matrix4. Como no
 * Vector3, os componentes podem ser lidos por nome (x, y, z, w) ou por índice
 * via o array @c v, pois compartilham a mesma memória (union).
 */
struct Vector4 {
    union {
        struct {
            float x, y, z, w;
        };
        float v[4]; ///< Acesso aos componentes por índice (compartilha memória com x,y,z,w).
    };

    /** @brief Soma componente a componente. */
    Vector4 operator+(const Vector4& other) const {
        return {x + other.x, y + other.y, z + other.z, w + other.w};
    }

    /** @brief Subtração componente a componente. */
    Vector4 operator-(const Vector4& other) const {
        return {x - other.x, y - other.y, z - other.z, w - other.w};
    }

    /** @brief Multiplicação por um escalar. */
    Vector4 operator*(const float scalar) const {
        return {x * scalar, y * scalar, z * scalar, w * scalar};
    }

    /** @brief Divisão por um escalar. */
    Vector4 operator/(const float scalar) const {
        return {x / scalar, y / scalar, z / scalar, w / scalar};
    }

    /** @brief Divisão por um escalar no próprio vetor (in-place). */
    Vector4& operator/=(const float scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
    }

    /** @brief Acesso a um componente por índice (0..3). */
    float& operator[](int i) { return v[i]; }
    /** @brief Acesso somente-leitura a um componente por índice (0..3). */
    const float& operator[](int i) const { return v[i]; }
};

/// @brief Constantes úteis de Vector4.
namespace VECTOR4 {
inline constexpr Vector4 ONES = {1.0f, 1.0f, 1.0f, 1.0f};  ///< (1, 1, 1, 1).
inline constexpr Vector4 ZEROS = {0.0f, 0.0f, 0.0f, 0.0f}; ///< (0, 0, 0, 0).
} // namespace VECTOR4

#endif // VECTOR4_HPP
