#ifndef COMPONENT_HPP
#define COMPONENT_HPP

class WorldObject;

/**
 * @brief Classe base de todos os componentes anexáveis a um WorldObject.
 *
 * Um componente adiciona comportamento ou dados a um objeto de cena (câmera,
 * luz, renderer, etc.). Toda subclasse herda daqui e é gerenciada pelo
 * WorldObject que a possui, acessível via getOwner().
 *
 * @see WorldObject
 */
class Component {
  protected:
    WorldObject* owner = nullptr; ///< Objeto que possui este componente.

  public:
    virtual ~Component() = default;

    /**
     * @brief Gancho de inicialização, chamado uma vez após a cena carregar.
     *
     * Padrão: não faz nada. Componentes que precisam preparar estado quando o
     * objeto já existe por completo (ex.: um script que lê o Transform do dono)
     * sobrescrevem este método. Chamado pela Application antes do primeiro
     * update.
     */
    virtual void start() {}

    /**
     * @brief Gancho de atualização por frame.
     * @param deltaTime Tempo decorrido desde o frame anterior, em segundos.
     *
     * Padrão: não faz nada. Componentes com comportamento por frame (ex.: um
     * ScriptComponent) sobrescrevem. Chamado pela Application a cada frame,
     * antes da renderização.
     */
    virtual void update(float deltaTime) { (void)deltaTime; }

    /** @brief Define o objeto dono deste componente (chamado por WorldObject). */
    void setOwner(WorldObject* obj) { owner = obj; }
    /** @brief Retorna o objeto que possui este componente. */
    WorldObject* getOwner() const { return owner; }
};

#endif
