#include "components/transform.hpp"
#include "math/math.hpp"

Matrix4 Transform::getModelMatrix() const {
    if (!dirty)
        return cachedMatrix;

    cachedMatrix = Matrix4(1.0f);
    cachedMatrix = Yume::Math::translate(cachedMatrix, {position.x, position.y, position.z});
    cachedMatrix =
        Yume::Math::rotate(cachedMatrix, Yume::Math::radians(rotation.x), {1.0f, 0.0f, 0.0f});
    cachedMatrix =
        Yume::Math::rotate(cachedMatrix, Yume::Math::radians(rotation.y), {0.0f, 1.0f, 0.0f});
    cachedMatrix =
        Yume::Math::rotate(cachedMatrix, Yume::Math::radians(rotation.z), {0.0f, 0.0f, 1.0f});
    cachedMatrix = Yume::Math::scale(cachedMatrix, {scale.x, scale.y, scale.z});
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