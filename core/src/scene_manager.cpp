#define CLASS_NAME "SceneManager"
#include "log_macros.hpp"

#include "renderer/renderer_backend.hpp"
#include "scene_loader.hpp"
#include "scene_manager.hpp"

void SceneManager::addScene(const std::string& name, const std::string& path) {
    sceneRegistry[name] = path;
}

Scene* SceneManager::getActiveScene() const { return activeScene.get(); }

void SceneManager::loadScene(const std::string& name) {
    auto it = sceneRegistry.find(name);
    if (it == sceneRegistry.end()) {
        LOG_WARN("Scene not found: " + name);
        return;
    }

    activeSceneName = name;
    activeScene = std::make_unique<Scene>();

    auto compiledScene = sceneLoader.loadCompiledScene(it->second);
    if (!compiledScene)
        return;

    // Carregar todos os world objects
    sceneLoader.loadWorldObjects(activeScene->getObjectManager(), compiledScene);

    // Encontrar e setar a camera principal
    for (auto& obj : activeScene->getObjectManager()->getObjects()) {
        if (obj->hasComponent<Camera>()) {
            activeScene->setCameraObject(obj.get());
            break;
        }
    }

    delete compiledScene;
}

void SceneManager::setRendererBackend(RendererBackend& rendererBackend) {
    sceneLoader.setRendererBackend(rendererBackend);
}
