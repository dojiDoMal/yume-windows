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

    /** @brief Define o objeto dono deste componente (chamado por WorldObject). */
    void setOwner(WorldObject* obj) { owner = obj; }
    /** @brief Retorna o objeto que possui este componente. */
    WorldObject* getOwner() const { return owner; }
};

#endif
