#include "transform.hpp"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Transform::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(scale.x, scale.y, scale.z));
    return model;
}

Vector3 Transform::getPosition() const { 
    return position; 
}

void Transform::setPosition(const Vector3& pos) { 
    position = pos; 
}

Vector3 Transform::getRotation() const { 
    return rotation; 
}

void Transform::setRotation(const Vector3& rot) { 
    rotation = rot; 
}

Vector3 Transform::getScale() const { 
    return scale; 
}

void Transform::setScale(const Vector3& scl) { 
    scale = scl; 
}