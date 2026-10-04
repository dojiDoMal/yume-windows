#ifndef SCENE_HPP
#define SCENE_HPP

#include "components/camera.hpp"
#include "world_object.hpp"
#include "world_object_manager.hpp"
#include <memory>

/**
 * @brief Uma cena carregada: o conjunto de objetos e a câmera ativa.
 *
 * Guarda um WorldObjectManager (dono de todos os objetos) e aponta para o
 * objeto que atua como câmera. Oferece atalhos para extrair subconjuntos de
 * objetos úteis à renderização (os que têm luz, os renderizáveis).
 *
 * @see SceneManager, WorldObjectManager, Camera
 */
class Scene {
  private:
    std::unique_ptr<WorldObjectManager> objectManager; ///< Dono dos objetos da cena.
    // TODO: adicionar suporte para mais de uma camera
    WorldObject* cameraObject = nullptr;                ///< Objeto que carrega a câmera ativa.

  public:
    Scene();
    ~Scene() = default;

    /** @brief Retorna o gerenciador de objetos da cena. */
    WorldObjectManager* getObjectManager();
    /** @brief Versão const de getObjectManager(). */
    const WorldObjectManager* getObjectManager() const;

    /** @brief Define qual objeto é a câmera ativa. */
    void setCameraObject(WorldObject* obj);
    /** @brief Retorna o objeto definido como câmera ativa. */
    WorldObject* getCameraObject() const;
    /** @brief Atalho para o componente Camera do objeto de câmera ativo. */
    Camera* getCamera() const;

    /** @brief Retorna todos os objetos que possuem um componente de luz. */
    std::vector<WorldObject*> getLightObjects() const;

    /** @brief Retorna todos os objetos renderizáveis (com MeshRenderer). */
    std::vector<WorldObject*> getRenderableObjects() const;
};

#endif
