#ifndef VECTOR3_HPP
#define VECTOR3_HPP

/**
 * @brief Vetor de 3 componentes de ponto flutuante (x, y, z).
 *
 * Usado para posições, direções, escalas e normais no espaço 3D. Os componentes
 * podem ser acessados por nome (x, y, z) ou por índice via o array @c v, pois
 * ambos compartilham a mesma memória (union).
 *
 * Constantes de direção prontas estão no namespace ::VECTOR3 (ex.: VECTOR3::UP),
 * assumindo um sistema de coordenadas destro (right-handed).
 */
struct Vector3 {
    union {
        struct {
            float x, y, z;
        };
        float v[3]; ///< Acesso aos componentes por índice (compartilha memória com x,y,z).
    };

    /** @brief Soma componente a componente. */
    Vector3 operator+(const Vector3& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    /** @brief Subtração componente a componente. */
    Vector3 operator-(const Vector3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    /** @brief Multiplicação por um escalar. */
    Vector3 operator*(const float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
};

/// @brief Vetores unitários e constantes úteis de Vector3 (sistema destro).
namespace VECTOR3 {
inline constexpr Vector3 UP = {0.0f, 1.0f, 0.0f};     ///< Para cima (+Y).
inline constexpr Vector3 DOWN = {0.0f, -1.0f, 0.0f};  ///< Para baixo (-Y).
inline constexpr Vector3 RIGHT = {1.0f, 0.0f, 0.0f};  ///< Para a direita (+X).
inline constexpr Vector3 LEFT = {-1.0f, 0.0f, 0.0f};  ///< Para a esquerda (-X).
inline constexpr Vector3 BACK = {0.0f, 0.0f, 1.0f};   ///< Para trás (+Z).
inline constexpr Vector3 FRONT = {0.0f, 0.0f, -1.0f}; ///< Para frente (-Z).
inline constexpr Vector3 ONES = {1.0f, 1.0f, 1.0f};   ///< (1, 1, 1).
inline constexpr Vector3 ZEROS = {0.0f, 0.0f, 0.0f};  ///< (0, 0, 0).
} // namespace VECTOR3

#endif // VECTOR3_HPP
