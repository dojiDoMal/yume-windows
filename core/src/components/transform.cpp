#include "components/transform.hpp"
#include "math/math.hpp"

Matrix4 Transform::getModelMatrix() const {
    if (!dirty)
        return cachedMatrix;

    // M = T · R(quat) · S. A rotação vem do quatérnion (orientação canônica);
    // a matriz de rotação sai idêntica à antiga composição Rx·Ry·Rz quando a
    // orientação foi construída de um Euler (ver Yume::Math::quatFromEuler).
    cachedMatrix = Matrix4(1.0f);
    cachedMatrix = Yume::Math::translate(cachedMatrix, {position.x, position.y, position.z});
    cachedMatrix = cachedMatrix * Yume::Math::quatToMatrix4(orientation);
    cachedMatrix = Yume::Math::scale(cachedMatrix, {scale.x, scale.y, scale.z});
    dirty = false;
    return cachedMatrix;
}

Vector3 Transform::getPosition() const { return position; }

void Transform::setPosition(const Vector3& pos) {
    position = pos;
    dirty = true;
}

Vector3 Transform::getRotation() const {
    // Se a orientação foi setada por quatérnion (física), o Euler em cache está
    // defasado: reconstrói a partir do quatérnion. Caso contrário devolve o
    // Euler exato que foi setado (edição por eixo estável).
    if (eulerStale) {
        cachedEuler = Yume::Math::quatToEuler(orientation);
        eulerStale = false;
    }
    return cachedEuler;
}

void Transform::setRotation(const Vector3& rot) {
    cachedEuler = rot;
    eulerStale = false;
    orientation = Yume::Math::quatFromEuler(rot);
    dirty = true;
}

Quaternion Transform::getRotationQuat() const { return orientation; }

void Transform::setRotationQuat(const Quaternion& q) {
    orientation = Yume::Math::normalize(q);
    eulerStale = true; // getRotation() vai reconstruir o Euler sob demanda.
    dirty = true;
}

Vector3 Transform::getScale() const { return scale; }

void Transform::setScale(const Vector3& scl) {
    scale = scl;
    dirty = true;
}
