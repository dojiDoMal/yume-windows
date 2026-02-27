#ifndef GAME_OBJECT_HPP
#define GAME_OBJECT_HPP

#include "mesh.hpp"
#include "mesh_renderer.hpp"
#include "sprite.hpp"
#include "sprite_renderer.hpp"
#include "transform.hpp"
#include <memory>

class GameObject {
  private:
    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<MeshRenderer> meshRenderer;
    std::unique_ptr<Sprite> sprite;
    std::unique_ptr<SpriteRenderer> spriteRenderer;

    std::unique_ptr<Transform> transform;

  public:
    GameObject() = default;

    void setTransform(std::unique_ptr<Transform> t);
    Transform* getTransform();

    void setMesh(std::unique_ptr<Mesh> m);
    Mesh* getMesh();
    const Mesh* getMesh() const;
    bool hasMesh() const;

    void setMeshRenderer(std::unique_ptr<MeshRenderer> m);
    MeshRenderer* getMeshRenderer();
    const MeshRenderer* getMeshRenderer() const;
    bool hasMeshRenderer() const;

    void setSprite(std::unique_ptr<Sprite> s);
    Sprite* getSprite();
    const Sprite* getSprite() const;
    bool hasSprite() const;

    void setSpriteRenderer(std::unique_ptr<SpriteRenderer> sr);
    SpriteRenderer* getSpriteRenderer();
    const SpriteRenderer* getSpriteRenderer() const;
    bool hasSpriteRenderer() const;
};

#endif
