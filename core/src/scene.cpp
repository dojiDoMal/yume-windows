#include "scene.hpp"

Scene::~Scene() {
    if (gameObjects) {
        for (GameObject* obj : *gameObjects) {
            delete obj;
        }
        delete gameObjects;
    }

    if (lights != nullptr) {
        delete lights;
    }

    if (mainCamera != nullptr) {
        delete mainCamera;
    }
}

void Scene::setCamera(Camera* cam) { 
    mainCamera = cam; 
};

Camera* Scene::getCamera() const { 
    return mainCamera; 
};

void Scene::setGameObjects(std::vector<GameObject*>* gos) { 
    gameObjects = gos; 
};

std::vector<GameObject*>* Scene::getGameObjects() { 
    return gameObjects; 
};

const std::vector<GameObject*>* Scene::getGameObjects() const { 
    return gameObjects; 
};

void Scene::setLights(std::vector<Light>* l) { 
    lights = l; 
};

std::vector<Light>* Scene::getLights() { 
    return lights; 
};

const std::vector<Light>* Scene::getLights() const { 
    return lights; 
};