#ifndef SCENE_LOADER_HPP
#define SCENE_LOADER_HPP

#include "camera.hpp"
#include "game_object.hpp"
#include "light.hpp"
#include "mesh.hpp"
#include "renderer/renderer_backend.hpp"
#include "scene_format.hpp"
#include <memory>
#include <string>
#include <vector>

class SceneLoader {
  private:
    RendererBackend* rendererBackend = nullptr;

    std::unique_ptr<Mesh> loadObjMesh(const std::string& filepath, bool shadeSmooth);
    void loadTransformComponent(GameObject* gameObject, const ComponentData& comp);
    void loadMeshRendererComponent(GameObject* gameObject, const ComponentData& comp);
    void loadSpriteRendererComponent(GameObject* gameObject, const ComponentData& comp);

  public:
    SceneLoader();
    void setRendererBackend(RendererBackend&);
    bool validateSceneFile(const std::string& filepath);
    CompiledScene* loadCompiledScene(const std::string& filepath);

    Camera* loadCamera(const CompiledScene* scene);
    std::vector<GameObject*>* loadGameObjects(const CompiledScene* scene);
    std::vector<Light>* loadLights(const CompiledScene* scene);
};

#endif
