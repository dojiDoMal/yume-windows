#include "game_object.hpp"

void GameObject::setTransform(std::unique_ptr<Transform> t) { 
    transform = std::move(t); 
}

Transform* GameObject::getTransform() { 
    return transform.get(); 
}

void GameObject::setMesh(std::unique_ptr<Mesh> m) { 
    mesh = std::move(m); 
}

Mesh* GameObject::getMesh() { 
    return mesh.get(); 
}

const Mesh* GameObject::getMesh() const { 
    return mesh.get(); 
}

bool GameObject::hasMesh() const { 
    return mesh != nullptr; 
}

void GameObject::setMeshRenderer(std::unique_ptr<MeshRenderer> m) { 
    meshRenderer = std::move(m); 
}

MeshRenderer* GameObject::getMeshRenderer() { 
    return meshRenderer.get(); 
}

const MeshRenderer* GameObject::getMeshRenderer() const { 
    return meshRenderer.get(); 
}

bool GameObject::hasMeshRenderer() const { 
    return meshRenderer != nullptr; 
}

void GameObject::setSprite(std::unique_ptr<Sprite> s) { 
    sprite = std::move(s); 
}

Sprite* GameObject::getSprite() { 
    return sprite.get(); 
}

const Sprite* GameObject::getSprite() const { 
    return sprite.get(); 
}

bool GameObject::hasSprite() const { 
    return sprite != nullptr; 
}

void GameObject::setSpriteRenderer(std::unique_ptr<SpriteRenderer> sr) { 
    spriteRenderer = std::move(sr); 
}

SpriteRenderer* GameObject::getSpriteRenderer() { 
    return spriteRenderer.get(); 
}

const SpriteRenderer* GameObject::getSpriteRenderer() const { 
    return spriteRenderer.get(); 
}

bool GameObject::hasSpriteRenderer() const { 
    return spriteRenderer != nullptr; 
}