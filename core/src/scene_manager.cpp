#define CLASS_NAME "SceneManager"
#include "scene_manager.hpp"
#include "log_macros.hpp"
#include "renderer/renderer_backend.hpp"
#include "scene_loader.hpp"


SceneManager::~SceneManager() {
    if (activeScene != nullptr) {
        delete activeScene;
    }
}

void SceneManager::addScene(const std::string& name, const std::string& path) {
    sceneRegistry[name] = path;
}

Scene* SceneManager::getActiveScene() const { return activeScene; }

// TODO: revisar esse delete
void SceneManager::loadScene(const std::string& name) {
    auto it = sceneRegistry.find(name);
    if (it == sceneRegistry.end()) {
        LOG_WARN("Scene not found: " + name);
        return;
    }

    if (activeScene != nullptr) {
        delete activeScene;
    }

    activeSceneName = name;
    activeScene = new Scene();
    
    auto compiledScene = sceneLoader.loadCompiledScene(it->second);
    if (!compiledScene) return;
    
    activeScene->setCamera(sceneLoader.loadCamera(compiledScene));
    activeScene->setLights(sceneLoader.loadLights(compiledScene));
    activeScene->setGameObjects(sceneLoader.loadGameObjects(compiledScene));

    delete compiledScene;
}

void SceneManager::setRendererBackend(RendererBackend& rendererBackend) {
    sceneLoader.setRendererBackend(rendererBackend);
}