#ifndef YUME_MATH_HPP
#define YUME_MATH_HPP

#include "math/matrix3.hpp"
#include "math/matrix4.hpp"
#include "math/quaternion.hpp"
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

    // -------------------------------------------------------------------------
    // Quatérnions. A convenção de Euler casa com a composição do Transform
    // (Rx·Ry·Rz, ângulos em GRAUS na API pública): eulerDegrees.{x,y,z} são as
    // rotações em torno de X, Y, Z, aplicadas nessa ordem.
    // -------------------------------------------------------------------------

    /** @brief Comprimento (norma) de um quatérnion. */
    static float length(const Quaternion& q);
    /** @brief Quatérnion unitário na mesma direção (identidade se norma ~0). */
    static Quaternion normalize(const Quaternion& q);
    /** @brief Conjugado (inverso de um quatérnion unitário): (-x,-y,-z,w). */
    static Quaternion conjugate(const Quaternion& q);
    /** @brief Produto escalar entre dois quatérnions (4 componentes). */
    static float dot(const Quaternion& a, const Quaternion& b);

    /**
     * @brief Constrói um quatérnion a partir de ângulos de Euler em GRAUS.
     * @param eulerDegrees Ângulos (x,y,z) em torno de X, Y, Z. Ordem Rx·Ry·Rz,
     *                     a mesma do Transform::getModelMatrix.
     */
    static Quaternion quatFromEuler(const Vector3& eulerDegrees);
    /**
     * @brief Extrai ângulos de Euler em GRAUS de um quatérnion (ordem Rx·Ry·Rz).
     *
     * É a inversa aproximada de quatFromEuler. Como a decomposição de Euler não
     * é única, use apenas para exibição/compatibilidade — a representação
     * canônica da orientação é o próprio quatérnion.
     */
    static Vector3 quatToEuler(const Quaternion& q);

    /** @brief Quatérnion de uma rotação de @p angleRadians em torno de @p axis. */
    static Quaternion quatFromAxisAngle(const Vector3& axis, float angleRadians);

    /** @brief Matriz de rotação 3x3 equivalente ao quatérnion (unitário). */
    static Matrix3 quatToMatrix3(const Quaternion& q);
    /** @brief Matriz de rotação 4x4 equivalente ao quatérnion (unitário). */
    static Matrix4 quatToMatrix4(const Quaternion& q);

    /** @brief Gira o vetor @p v pela rotação do quatérnion @p q. */
    static Vector3 rotateVector(const Quaternion& q, const Vector3& v);

    /**
     * @brief Integra a orientação por um passo de velocidade angular.
     * @param q               Orientação atual (unitária).
     * @param angularVelocity Velocidade angular em rad/s (eixo * magnitude).
     * @param dt              Passo de tempo, em segundos.
     * @return A nova orientação, já normalizada.
     *
     * Usa a forma padrão q' = normalize(q + 0.5·ω·q·dt), com ω tratado como
     * quatérnion puro (w=0). Estável para passos pequenos.
     */
    static Quaternion integrate(const Quaternion& q, const Vector3& angularVelocity, float dt);
};
} // namespace Yume

#endif // YUME_MATH_HPP
