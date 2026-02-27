#ifndef SCENE_HPP
#define SCENE_HPP

#include "camera.hpp"
#include "game_object.hpp"
#include "light.hpp"

class Scene {
  private:
    Camera* mainCamera = nullptr;
    std::vector<GameObject*>* gameObjects = nullptr;
    std::vector<Light>* lights = nullptr;

  public:
    ~Scene();
    void setCamera(Camera* cam);
    Camera* getCamera() const;

    void setGameObjects(std::vector<GameObject*>* gos);
    std::vector<GameObject*>* getGameObjects();
    const std::vector<GameObject*>* getGameObjects() const;

    void setLights(std::vector<Light>* l);
    std::vector<Light>* getLights();
    const std::vector<Light>* getLights() const;
};

#endif