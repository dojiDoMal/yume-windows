#ifndef YUME_MATH_HPP
#define YUME_MATH_HPP

#include "math/matrix4.hpp"
#include "math/vector3.hpp"

namespace Yume {

/**
 * @brief Funções utilitárias de álgebra linear e construção de matrizes.
 *
 * Reúne operações sobre Vector3 (produto escalar, produto vetorial, norma,
 * normalização) e geradores das matrizes de transformação usadas pelo
 * renderer: translação, rotação, escala, view (lookAt) e projeções
 * (perspectiva e ortográfica). As convenções seguem glm, com variantes
 * explícitas @c *RH_ZO (right-handed, profundidade mapeada em [0, 1]) para
 * backends como Vulkan/DirectX.
 */
class Math {
  public:
    static constexpr float PI = 3.14159265358979323846f; ///< Constante π.

    /** @brief Produto escalar entre dois vetores. */
    static float dot(const Vector3& a, const Vector3& b);
    /** @brief Produto vetorial a × b (resulta num vetor perpendicular a ambos). */
    static Vector3 cross(const Vector3& a, const Vector3& b);
    /** @brief Comprimento (norma euclidiana) do vetor. */
    static float length(const Vector3& a);
    /** @brief Vetor unitário na mesma direção de @p a. */
    static Vector3 normalize(const Vector3& a);
    /** @brief Comprimento ao quadrado (evita a raiz; útil para comparações). */
    static float lengthSquared(const Vector3& a);
    /** @brief Mínimo componente a componente entre dois vetores. */
    static Vector3 min(const Vector3& a, const Vector3& b);
    /** @brief Máximo componente a componente entre dois vetores. */
    static Vector3 max(const Vector3& a, const Vector3& b);
    /** @brief Converte um ângulo de graus para radianos. */
    static float radians(float degrees);

    /** @brief Aplica uma translação @p v à matriz @p m. */
    static Matrix4 translate(const Matrix4& m, const Vector3& v);
    /**
     * @brief Aplica uma rotação à matriz @p m.
     * @param m           Matriz de base.
     * @param angleRadians Ângulo de rotação, em radianos.
     * @param axis        Eixo de rotação (deve ser unitário).
     */
    static Matrix4 rotate(const Matrix4& m, float angleRadians, const Vector3& axis);
    /** @brief Aplica uma escala @p v à matriz @p m. */
    static Matrix4 scale(const Matrix4& m, const Vector3& v);
    /**
     * @brief Constrói uma matriz de visão (câmera).
     * @param eye    Posição da câmera.
     * @param center Ponto para onde a câmera olha.
     * @param up     Direção "para cima" da câmera.
     */
    static Matrix4 lookAt(const Vector3& eye, const Vector3& center, const Vector3& up);
    /**
     * @brief Matriz de projeção em perspectiva.
     * @param fovyRadians Campo de visão vertical, em radianos.
     * @param aspect      Razão de aspecto (largura/altura).
     * @param zNear       Distância do plano de corte próximo.
     * @param zFar        Distância do plano de corte distante.
     */
    static Matrix4 perspective(float fovyRadians, float aspect, float zNear, float zFar);
    /** @brief Perspectiva right-handed com profundidade em [0, 1] (Vulkan/DirectX). */
    static Matrix4 perspectiveRH_ZO(float fovyRadians, float aspect, float zNear, float zFar);
    /** @brief Projeção ortográfica com planos near/far explícitos. */
    static Matrix4 ortho(float left, float right, float bottom, float top, float zNear, float zFar);
    /** @brief Projeção ortográfica 2D (sem profundidade), útil para UI/sprites. */
    static Matrix4 ortho(float left, float right, float bottom, float top);
    /** @brief Ortográfica right-handed com profundidade em [0, 1] (Vulkan/DirectX). */
    static Matrix4 orthoRH_ZO(float left, float right, float bottom, float top, float zNear,
                              float zFar);
};
} // namespace Yume

#endif // YUME_MATH_HPP
