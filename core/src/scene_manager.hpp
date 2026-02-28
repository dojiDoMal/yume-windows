#ifndef SCENE_MANAGER_HPP
#define SCENE_MANAGER_HPP

#include "renderer/renderer_backend.hpp"
#include "scene.hpp"
#include "scene_loader.hpp"
#include <memory>
#include <string>
#include <unordered_map>

class SceneManager {
  private:
    std::unordered_map<std::string, std::string> sceneRegistry;
    std::string activeSceneName;
    std::unique_ptr<Scene> activeScene;
    SceneLoader sceneLoader;

  public:
    SceneManager() = default;
    ~SceneManager() = default;

    void addScene(const std::string& name, const std::string& path);
    void loadScene(const std::string& name);
    void setRendererBackend(RendererBackend& rendererBackend);
    Scene* getActiveScene() const;
};

#endif
