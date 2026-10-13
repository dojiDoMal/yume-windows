#ifndef SPHERE_COLLIDER_HPP
#define SPHERE_COLLIDER_HPP

#include "components/component.hpp"
#include "math/vector3.hpp"

/**
 * @brief Volume de colisão em forma de esfera.
 *
 * Descreve uma esfera de raio @c radius centrada em @c center (deslocamento
 * relativo à posição do objeto). É um componente de dados puro: a detecção e a
 * resolução de colisão são feitas pelo PhysicsWorld, que consulta estes valores
 * junto ao Transform do dono.
 *
 * Diferente da caixa (AABB), a esfera é invariante à rotação, então não há
 * aproximação envolvida no caso de o objeto girar.
 *
 * @see Component, PhysicsWorld, BoxCollider, PlaneCollider, Rigidbody
 */
class SphereCollider : public Component {
  private:
    float radius = 0.5f;                 ///< Raio da esfera.
    Vector3 center = {0.0f, 0.0f, 0.0f}; ///< Centro, relativo à posição do dono.

  public:
    SphereCollider() = default;

    /** @brief Define o raio da esfera (clampeado para um mínimo positivo). */
    void setRadius(float r) { radius = (r > 0.0001f) ? r : 0.0001f; }
    /** @brief Retorna o raio da esfera. */
    float getRadius() const { return radius; }

    /** @brief Define o centro da esfera, relativo à posição do dono. */
    void setCenter(const Vector3& c) { center = c; }
    /** @brief Retorna o centro da esfera, relativo à posição do dono. */
    const Vector3& getCenter() const { return center; }
};

#endif // SPHERE_COLLIDER_HPP
