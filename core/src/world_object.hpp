#ifndef WORLD_OBJECT_HPP
#define WORLD_OBJECT_HPP

#include "components/component.hpp"
#include "mesh.hpp"
#include "sprite.hpp"
#include "transform.hpp"
#include <memory>
#include <typeinfo>
#include <vector>

class WorldObject {
  private:
    Transform transform;
    std::vector<std::unique_ptr<Component>> components;

    // TODO: Remover uso de mesh e sprite diretamente
    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<Sprite> sprite;

  public:
    WorldObject() = default;

    // TODO: esse gettransform poderia por baixo dos panos chamar getComponent<Transform>
    // e o transform ficar dentro do vetor de componentes ?? só teria um transform mesmo...
    Transform& getTransform();
    const Transform& getTransform() const;

    template <typename T> T* addComponent(std::unique_ptr<T> component) {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        T* ptr = component.get();
        ptr->setOwner(this);
        components.push_back(std::move(component));
        return ptr;
    }

    template <typename T> T* getComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        for (auto& comp : components) {
            if (T* ptr = dynamic_cast<T*>(comp.get())) {
                return ptr;
            }
        }
        return nullptr;
    }

    template <typename T> const T* getComponent() const {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        for (auto& comp : components) {
            if (const T* ptr = dynamic_cast<const T*>(comp.get())) {
                return ptr;
            }
        }
        return nullptr;
    }

    template <typename T> bool hasComponent() const { return getComponent<T>() != nullptr; }

    // TODO: remover suporte a legacy mesh/sprite
    void setMesh(std::unique_ptr<Mesh> m) { mesh = std::move(m); }
    Mesh* getMesh() { return mesh.get(); }
    const Mesh* getMesh() const { return mesh.get(); }
    bool hasMesh() const { return mesh != nullptr; }

    void setSprite(std::unique_ptr<Sprite> s) { sprite = std::move(s); }
    Sprite* getSprite() { return sprite.get(); }
    const Sprite* getSprite() const { return sprite.get(); }
    bool hasSprite() const { return sprite != nullptr; }
};

#endif
