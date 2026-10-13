#ifndef PHYSICS_WORLD_HPP
#define PHYSICS_WORLD_HPP

#include "math/vector3.hpp"

class WorldObjectManager;

/**
 * @brief Motor de física simples da cena: gravidade e resolução de colisão.
 *
 * O PhysicsWorld é o "sistema" do par entidade-componente para física. A cada
 * passo (step) ele varre os objetos do WorldObjectManager e, para cada corpo
 * com Rigidbody:
 *   1. integra a gravidade na velocidade e move o Transform do dono;
 *   2. detecta e resolve colisões contra os colliders estáticos (PlaneCollider)
 *      e contra outros corpos (BoxCollider vs BoxCollider).
 *
 * A simulação só avança quando a física está habilitada (ver setEnabled). Isso
 * permite que um editor controle o botão "play": com a física desligada a cena
 * fica parada no estado de autoria.
 *
 * As caixas são tratadas como AABB (sem rotação) — suficiente para o caso de um
 * cubo caindo sobre um plano. O objetivo aqui é um comportamento simples e
 * previsível, não um motor físico completo.
 *
 * @see Rigidbody, BoxCollider, PlaneCollider, WorldObjectManager
 */
class PhysicsWorld {
  private:
    Vector3 gravity = {0.0f, -9.81f, 0.0f}; ///< Aceleração da gravidade (unidades/s²).
    bool enabled = false;                   ///< Se @c false, step() não faz nada.

    // Atenuação (damping) por segundo: fração da velocidade mantida a cada
    // segundo. 0.0 = sem atenuação; valores < 1 dissipam energia como o atrito
    // do ar / rolamento, evitando que ω cresça indefinidamente (o "giro louco").
    // Aplicados exponencialmente por frame: v *= pow(1-damping, dt).
    float linearDamping = 0.05f;  ///< Atenuação da velocidade linear por segundo.
    float angularDamping = 0.30f; ///< Atenuação da velocidade angular por segundo.

    // Limite de segurança para a velocidade angular (rad/s). Impede que um
    // contato numérico ruim exploda o giro.
    float maxAngularSpeed = 20.0f;

  public:
    PhysicsWorld() = default;

    /** @brief Define o vetor de gravidade global. */
    void setGravity(const Vector3& g) { gravity = g; }
    /** @brief Retorna o vetor de gravidade global. */
    const Vector3& getGravity() const { return gravity; }

    /** @brief Habilita ou desabilita a simulação (botão "play" do editor). */
    void setEnabled(bool e) { enabled = e; }
    /** @brief Indica se a simulação está habilitada. */
    bool isEnabled() const { return enabled; }

    /** @brief Define a atenuação linear por segundo (0 = nenhuma). */
    void setLinearDamping(float d) { linearDamping = d; }
    /** @brief Define a atenuação angular por segundo (0 = nenhuma). */
    void setAngularDamping(float d) { angularDamping = d; }
    /** @brief Define o teto de velocidade angular (rad/s). */
    void setMaxAngularSpeed(float s) { maxAngularSpeed = s; }

    /**
     * @brief Avança a simulação um passo de tempo.
     * @param manager   Fonte dos objetos (e seus componentes de física).
     * @param deltaTime Tempo do passo, em segundos.
     *
     * Não faz nada se a física estiver desabilitada ou @p manager for nulo.
     */
    void step(WorldObjectManager* manager, float deltaTime);
};

#endif // PHYSICS_WORLD_HPP
