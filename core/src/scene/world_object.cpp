#include "scene/world_object.hpp"

#include <algorithm>

Transform& WorldObject::getTransform() { return transform; }

const Transform& WorldObject::getTransform() const { return transform; }

void WorldObject::setParent(WorldObject* newParent) {
    if (newParent == parent)
        return;

    // Desvincula do pai antigo (remove da lista de filhos dele).
    if (parent) {
        auto& sib = parent->children;
        sib.erase(std::remove(sib.begin(), sib.end(), this), sib.end());
    }

    parent = newParent;

    // Vincula ao novo pai.
    if (parent)
        parent->children.push_back(this);
}

Matrix4 WorldObject::getWorldMatrix() const {
    Matrix4 local = transform.getModelMatrix();
    if (!parent)
        return local;
    return parent->getWorldMatrix() * local;
}

Vector3 WorldObject::getWorldPosition() const {
    // A translação de mundo é a quarta coluna da world matrix (column-major).
    Matrix4 world = getWorldMatrix();
    return Vector3{world[3].x, world[3].y, world[3].z};
}

Vector3 WorldObject::getWorldRotation() const {
    Vector3 r = transform.getRotation();
    if (!parent)
        return r;
    // Soma por eixo subindo a cadeia (aproximação Euler; ver nota no header).
    Vector3 pr = parent->getWorldRotation();
    return Vector3{r.x + pr.x, r.y + pr.y, r.z + pr.z};
}
