#ifndef RIGIDBODY_HPP
#define RIGIDBODY_HPP

#include "components/component.hpp"
#include "math/vector3.hpp"

/**
 * @brief Corpo rígido: dá a um objeto massa, velocidade e resposta à gravidade.
 *
 * Anexado a um WorldObject que também tenha um collider, faz o objeto participar
 * da simulação física: a cada passo o PhysicsWorld integra a gravidade na
 * velocidade (linear e angular), move/gira o Transform do dono e resolve
 * colisões contra outros colliders da cena.
 *
 * Além do estado linear (velocidade, massa), guarda o estado ROTACIONAL:
 * velocidade angular e o inverso do momento de inércia (diagonal do tensor em
 * espaço do corpo, isotrópico na esfera e por-eixo na caixa). O PhysicsWorld
 * calcula a inércia a partir da forma do collider uma vez (setupInertia) e usa
 * impulsos com braço de alavanca para gerar torque no contato — é isso que faz
 * a esfera rolar e o cubo bambear.
 *
 * A integração em si é feita pelo PhysicsWorld, não no update() virtual, para
 * centralizar a ordem "integrar → detectar → resolver" num único lugar.
 *
 * @see Component, PhysicsWorld, BoxCollider, SphereCollider, PlaneCollider
 */
class Rigidbody : public Component {
  private:
    Vector3 velocity = {0.0f, 0.0f, 0.0f};        ///< Velocidade linear (unidades/s).
    Vector3 angularVelocity = {0.0f, 0.0f, 0.0f}; ///< Velocidade angular (rad/s, eixo*magnitude).
    float mass = 1.0f;                            ///< Massa do corpo (kg). Deve ser > 0.
    float gravityScale = 1.0f;                    ///< Multiplicador da gravidade global.
    float restitution = 0.0f;                     ///< Elasticidade do quique [0,1].
    float friction = 0.5f;                        ///< Atrito de contato [0,1+]. Gera o rolamento.
    bool useGravity = true;                       ///< Se @c false, a gravidade não é aplicada.

    // Inverso do momento de inércia (diagonal do tensor, espaço do corpo). Zero
    // = rotação "travada" naquele eixo (padrão até o PhysicsWorld configurar a
    // partir do collider). Guardar o INVERSO evita divisões no laço de resolução.
    Vector3 inverseInertia = {0.0f, 0.0f, 0.0f};

  public:
    Rigidbody() = default;

    /** @brief Define a velocidade linear atual. */
    void setVelocity(const Vector3& v) { velocity = v; }
    /** @brief Retorna a velocidade linear atual. */
    const Vector3& getVelocity() const { return velocity; }

    /** @brief Define a velocidade angular (rad/s, eixo * magnitude). */
    void setAngularVelocity(const Vector3& w) { angularVelocity = w; }
    /** @brief Retorna a velocidade angular atual. */
    const Vector3& getAngularVelocity() const { return angularVelocity; }

    /** @brief Define a massa (clampeada para um mínimo positivo). */
    void setMass(float m) { mass = (m > 0.0001f) ? m : 0.0001f; }
    /** @brief Retorna a massa do corpo. */
    float getMass() const { return mass; }
    /** @brief Inverso da massa (0 trataria o corpo como infinito; aqui sempre > 0). */
    float getInverseMass() const { return 1.0f / mass; }

    /** @brief Define o multiplicador da gravidade global para este corpo. */
    void setGravityScale(float s) { gravityScale = s; }
    /** @brief Retorna o multiplicador da gravidade. */
    float getGravityScale() const { return gravityScale; }

    /** @brief Define a elasticidade do quique no intervalo [0,1]. */
    void setRestitution(float r) { restitution = (r < 0.0f) ? 0.0f : (r > 1.0f ? 1.0f : r); }
    /** @brief Retorna a elasticidade do quique. */
    float getRestitution() const { return restitution; }

    /** @brief Define o coeficiente de atrito de contato (>= 0). */
    void setFriction(float f) { friction = (f < 0.0f) ? 0.0f : f; }
    /** @brief Retorna o coeficiente de atrito de contato. */
    float getFriction() const { return friction; }

    /** @brief Liga/desliga a aplicação de gravidade neste corpo. */
    void setUseGravity(bool g) { useGravity = g; }
    /** @brief Indica se a gravidade é aplicada a este corpo. */
    bool getUseGravity() const { return useGravity; }

    /** @brief Define o inverso do momento de inércia (diagonal, espaço do corpo). */
    void setInverseInertia(const Vector3& invI) { inverseInertia = invI; }
    /** @brief Retorna o inverso do momento de inércia (diagonal). */
    const Vector3& getInverseInertia() const { return inverseInertia; }
};

#endif // RIGIDBODY_HPP
