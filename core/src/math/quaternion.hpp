#ifndef QUATERNION_HPP
#define QUATERNION_HPP

#include "math/vector3.hpp"

/**
 * @brief Quatérnion unitário para representar orientação/rotação 3D.
 *
 * Guarda a parte vetorial (x, y, z) e a escalar (w), convenção @c {w, x, y, z}
 * com w real. Comparado a ângulos de Euler, o quatérnion compõe rotações sem
 * gimbal lock e interpola de forma estável — é a representação interna de
 * rotação do Transform e a base da dinâmica rotacional da física.
 *
 * A identidade (sem rotação) é @c {x=0, y=0, z=0, w=1}. Operações que constroem
 * a partir de Euler/eixo-ângulo e convertem para matriz ficam em @ref Yume::Math,
 * seguindo a convenção do módulo (tipos globais, funções em Math).
 *
 * @see Yume::Math, Transform
 */
struct Quaternion {
    float x = 0.0f; ///< Componente vetorial i.
    float y = 0.0f; ///< Componente vetorial j.
    float z = 0.0f; ///< Componente vetorial k.
    float w = 1.0f; ///< Componente escalar (real).

    constexpr Quaternion() = default;
    constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    /**
     * @brief Produto de Hamilton (composição de rotações).
     *
     * @c a*b aplica a rotação @p b primeiro e depois @p a (como matrizes:
     * R(a*b) = R(a)·R(b)).
     */
    Quaternion operator*(const Quaternion& r) const {
        return {
            w * r.x + x * r.w + y * r.z - z * r.y, // x
            w * r.y - x * r.z + y * r.w + z * r.x, // y
            w * r.z + x * r.y - y * r.x + z * r.w, // z
            w * r.w - x * r.x - y * r.y - z * r.z  // w
        };
    }

    /** @brief Soma componente a componente (usada na integração de ω). */
    Quaternion operator+(const Quaternion& o) const {
        return {x + o.x, y + o.y, z + o.z, w + o.w};
    }

    /** @brief Multiplicação por escalar. */
    Quaternion operator*(float s) const { return {x * s, y * s, z * s, w * s}; }
};

/// @brief Constantes úteis de Quaternion.
namespace QUATERNION {
inline constexpr Quaternion IDENTITY = {0.0f, 0.0f, 0.0f, 1.0f}; ///< Sem rotação.
} // namespace QUATERNION

#endif // QUATERNION_HPP
