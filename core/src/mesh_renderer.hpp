#ifndef MESH_RENDERER_HPP
#define MESH_RENDERER_HPP

#include "material.hpp"
#include <memory>

class MeshRenderer {
  private:
    std::unique_ptr<Material> material;

  public:
    MeshRenderer() = default;
    void setMaterial(std::unique_ptr<Material> m) { material = std::move(m); };
    Material* getMaterial() { return material.get(); }
    const Material* getMaterial() const { return material.get(); }
    bool hasMaterial() const { return material != nullptr; }
};

#endif // MESH_RENDERER_HPP