#ifndef BOX_COLLIDER_HPP
#define BOX_COLLIDER_HPP

#include "components/component.hpp"
#include "math/vector3.hpp"

/**
 * @brief Volume de colisão em forma de caixa alinhada aos eixos (AABB local).
 *
 * Descreve uma caixa centrada em @c center (deslocamento relativo à posição do
 * objeto) com meias-extensões @c halfExtents em cada eixo. É um componente de
 * dados puro: a detecção e a resolução de colisão são feitas pelo PhysicsWorld,
 * que consulta estes valores junto ao Transform do dono.
 *
 * Por ora o volume é tratado como AABB (sem rotação): a rotação do objeto não é
 * aplicada à caixa. Para o caso de uso atual (cubo caindo sobre um plano) isso
 * é suficiente; OBB fica como melhoria futura.
 *
 * @see Component, PhysicsWorld, PlaneCollider, Rigidbody
 */
class BoxCollider : public Component {
  private:
    Vector3 halfExtents = {0.5f, 0.5f, 0.5f}; ///< Metade do tamanho em cada eixo.
    Vector3 center = {0.0f, 0.0f, 0.0f};      ///< Centro, relativo à posição do dono.

  public:
    BoxCollider() = default;

    /** @brief Define as meias-extensões (metade do tamanho em cada eixo). */
    void setHalfExtents(const Vector3& he) { halfExtents = he; }
    /** @brief Retorna as meias-extensões da caixa. */
    const Vector3& getHalfExtents() const { return halfExtents; }

    /** @brief Define o centro da caixa, relativo à posição do dono. */
    void setCenter(const Vector3& c) { center = c; }
    /** @brief Retorna o centro da caixa, relativo à posição do dono. */
    const Vector3& getCenter() const { return center; }
};

#endif // BOX_COLLIDER_HPP
