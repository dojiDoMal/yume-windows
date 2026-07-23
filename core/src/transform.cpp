#include "transform.hpp"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Transform::getModelMatrix() const {
    if (!dirty)
        return cachedMatrix;

    cachedMatrix = glm::mat4(1.0f);
    cachedMatrix = glm::translate(cachedMatrix, glm::vec3(position.x, position.y, position.z));
    cachedMatrix = glm::rotate(cachedMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    cachedMatrix = glm::rotate(cachedMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    cachedMatrix = glm::rotate(cachedMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    cachedMatrix = glm::scale(cachedMatrix, glm::vec3(scale.x, scale.y, scale.z));
    dirty = false;
    return cachedMatrix;
}

Vector3 Transform::getPosition() const { return position; }

void Transform::setPosition(const Vector3& pos) {
    position = pos;
    dirty = true;
}

Vector3 Transform::getRotation() const { return rotation; }

void Transform::setRotation(const Vector3& rot) {
    rotation = rot;
    dirty = true;
}

Vector3 Transform::getScale() const { return scale; }

void Transform::setScale(const Vector3& scl) {
    scale = scl;
    dirty = true;
}