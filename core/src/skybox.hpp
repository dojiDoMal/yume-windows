#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include "material.hpp"
#include "mesh.hpp"
#include <memory>


class Skybox {
  private:
    std::unique_ptr<Mesh> cubeMesh;
    std::unique_ptr<Material> skyboxMaterial;
    unsigned int textureID = 0;

  public:
    Skybox();
    ~Skybox() = default;

    bool init();
    void setTextureID(unsigned int id) { textureID = id; }
    unsigned int getTextureID() const { return textureID; }
    Mesh* getMesh() const { return cubeMesh.get(); }
    Material* getMaterial() const { return skyboxMaterial.get(); }
    void setMaterial(std::unique_ptr<Material> material);
};

#endif
