#ifndef SCENE_LOADER_HPP
#define SCENE_LOADER_HPP

#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/mesh_renderer.hpp"
#include "components/sprite_renderer.hpp"
#include "mesh.hpp"
#include "renderer/renderer_backend.hpp"
#include "scene_format.hpp"
#include "world_object.hpp"
#include "world_object_manager.hpp"
#include <memory>
#include <string>


class SceneLoader {
  private:
    RendererBackend* rendererBackend = nullptr;

    std::unique_ptr<Mesh> loadObjMesh(const std::string& filepath, bool shadeSmooth);
    void loadMeshRendererComponent(WorldObject* obj, const ComponentData& comp);
    void loadSpriteRendererComponent(WorldObject* obj, const ComponentData& comp);
    void loadCameraComponent(WorldObject* obj, const ComponentData& comp); // ADICIONAR
    void loadLightComponent(WorldObject* obj, const ComponentData& comp);  // ADICIONAR

  public:
    SceneLoader();
    void setRendererBackend(RendererBackend&);
    bool validateSceneFile(const std::string& filepath);
    CompiledScene* loadCompiledScene(const std::string& filepath);

    void loadWorldObjects(WorldObjectManager* manager, const CompiledScene* scene);
};

#endif
