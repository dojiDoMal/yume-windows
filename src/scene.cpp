#include "scene.hpp"
#include "components/light.hpp"

Scene::Scene() : objectManager(std::make_unique<WorldObjectManager>()) {}

WorldObjectManager* Scene::getObjectManager() { return objectManager.get(); }

const WorldObjectManager* Scene::getObjectManager() const { return objectManager.get(); }

void Scene::setCameraObject(WorldObject* obj) { cameraObject = obj; }

WorldObject* Scene::getCameraObject() const { return cameraObject; }

Camera* Scene::getCamera() const {
    return cameraObject ? cameraObject->getComponent<Camera>() : nullptr;
}

std::vector<WorldObject*> Scene::getLightObjects() const {
    std::vector<WorldObject*> result;
    for (auto& obj : objectManager->getObjects()) {
        if (obj->hasComponent<Light>()) {
            result.push_back(obj.get());
        }
    }
    return result;
}

std::vector<WorldObject*> Scene::getRenderableObjects() const {
    std::vector<WorldObject*> result;
    for (auto& obj : objectManager->getObjects()) {
        if (obj->hasMesh() || obj->hasSprite()) {
            result.push_back(obj.get());
        }
    }
    return result;
}