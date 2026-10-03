#ifndef SCENE_HPP
#define SCENE_HPP

#include "components/camera.hpp"
#include "world_object.hpp"
#include "world_object_manager.hpp"
#include <memory>

class Scene {
  private:
    std::unique_ptr<WorldObjectManager> objectManager;
    // TODO: adicionar suporte para mais de uma camera
    WorldObject* cameraObject = nullptr;

  public:
    Scene();
    ~Scene() = default;

    WorldObjectManager* getObjectManager();
    const WorldObjectManager* getObjectManager() const;

    // Camera management
    void setCameraObject(WorldObject* obj);
    WorldObject* getCameraObject() const;
    Camera* getCamera() const;

    // Helper: Get all objects with Light component
    std::vector<WorldObject*> getLightObjects() const;

    // Helper: Get all objects with MeshRenderer
    std::vector<WorldObject*> getRenderableObjects() const;
};

#endif