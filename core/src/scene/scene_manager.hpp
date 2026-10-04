#ifndef SCENE_MANAGER_HPP
#define SCENE_MANAGER_HPP

#include "renderer/renderer_backend.hpp"
#include "scene/scene.hpp"
#include "scene/scene_loader.hpp"
#include <memory>
#include <string>
#include <unordered_map>

/**
 * @brief Registra cenas por nome e controla qual está ativa.
 *
 * Primeiro registram-se as cenas disponíveis com addScene (nome -> caminho do
 * arquivo compilado); depois loadScene troca a cena ativa, usando o SceneLoader
 * interno para ler e montar os objetos. Apenas uma cena fica ativa por vez.
 *
 * @code
 * manager.setRendererBackend(backend);
 * manager.addScene("menu", "scenes/menu.scnb");
 * manager.loadScene("menu");
 * Scene* s = manager.getActiveScene();
 * @endcode
 *
 * @see Scene, SceneLoader
 */
class SceneManager {
  private:
    std::unordered_map<std::string, std::string> sceneRegistry; ///< Nome -> caminho do arquivo.
    std::string activeSceneName;              ///< Nome da cena atualmente ativa.
    std::unique_ptr<Scene> activeScene;       ///< Cena ativa carregada.
    SceneLoader sceneLoader;                  ///< Carregador usado para montar as cenas.

  public:
    SceneManager() = default;
    ~SceneManager() = default;

    /**
     * @brief Registra uma cena disponível para carregamento.
     * @param name Nome lógico da cena.
     * @param path Caminho do arquivo de cena compilado (.scnb).
     */
    void addScene(const std::string& name, const std::string& path);

    /**
     * @brief Carrega e torna ativa a cena registrada com @p name.
     * @param name Nome previamente registrado com addScene().
     */
    void loadScene(const std::string& name);

    /** @brief Define o backend de renderização usado ao montar as cenas. */
    void setRendererBackend(RendererBackend& rendererBackend);

    /** @brief Retorna a cena ativa, ou @c nullptr se nenhuma foi carregada. */
    Scene* getActiveScene() const;
};

#endif
