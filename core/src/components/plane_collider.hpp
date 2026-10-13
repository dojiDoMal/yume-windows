#ifndef PLANE_COLLIDER_HPP
#define PLANE_COLLIDER_HPP

#include "components/component.hpp"
#include "math/vector3.hpp"

/**
 * @brief Volume de colisão em forma de plano infinito.
 *
 * O plano é definido por uma @c normal (unitária) e passa pela posição do
 * objeto dono mais um deslocamento @c offset ao longo dessa normal. Serve como
 * "chão" estático contra o qual corpos com Rigidbody colidem.
 *
 * É um componente de dados puro: a resolução de colisão é feita pelo
 * PhysicsWorld. Um plano não tem Rigidbody (é sempre estático).
 *
 * @see Component, PhysicsWorld, BoxCollider, Rigidbody
 */
class PlaneCollider : public Component {
  private:
    Vector3 normal = {0.0f, 1.0f, 0.0f}; ///< Normal do plano (unitária). Default: para cima.
    float offset = 0.0f;                 ///< Deslocamento ao longo da normal, a partir do dono.

  public:
    PlaneCollider() = default;

    /** @brief Define a normal do plano (idealmente unitária). */
    void setNormal(const Vector3& n) { normal = n; }
    /** @brief Retorna a normal do plano. */
    const Vector3& getNormal() const { return normal; }

    /** @brief Define o deslocamento do plano ao longo da normal. */
    void setOffset(float o) { offset = o; }
    /** @brief Retorna o deslocamento do plano ao longo da normal. */
    float getOffset() const { return offset; }
};

#endif // PLANE_COLLIDER_HPP
