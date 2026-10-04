#ifndef WORLD_OBJECT_MANAGER_HPP
#define WORLD_OBJECT_MANAGER_HPP

#include "scene/world_object.hpp"
#include <memory>
#include <vector>

/**
 * @brief Dono e fábrica de todos os WorldObject de uma cena.
 *
 * Mantém a posse dos objetos (via unique_ptr) e cuida do ciclo de vida deles.
 * Crie objetos com createObject(), itere com getObjects() e descarte todos com
 * clear(). Cada Scene possui um WorldObjectManager.
 */
class WorldObjectManager {
  private:
    std::vector<std::unique_ptr<WorldObject>> objects; ///< Objetos da cena (posse exclusiva).

  public:
    WorldObjectManager() = default;

    /**
     * @brief Cria um novo WorldObject já gerenciado por este manager.
     * @return Ponteiro para o objeto criado (a posse permanece no manager).
     */
    WorldObject* createObject();

    /** @brief Retorna a lista de todos os objetos para iteração. */
    const std::vector<std::unique_ptr<WorldObject>>& getObjects() const;

    /** @brief Remove e destrói todos os objetos. */
    void clear();
};

#endif
