#include "camera.hpp"

const Vector3& Camera::getPosition() const { return position; }

void Camera::setPosition(const Vector3& position) { this->position = position; }

const Vector3& Camera::getTarget() const { return target; }

void Camera::setTarget(const Vector3& target) { this->target = target; }

ColorRGBA& Camera::getBackgroundColor() { return backgroundColor; }

void Camera::setBackgroundColor(const ColorRGBA& color) { this->backgroundColor = color; }

void Camera::setFov(float fov) { this->fov = fov; }

float Camera::getFov() const { return fov; }

void Camera::setNearDistance(float nearDistance) { this->nearDistance = nearDistance; }

float Camera::getNearDistance() const { return nearDistance; }

void Camera::setFarDistance(float farDistance) { this->farDistance = farDistance; }

float Camera::getFarDistance() const { return farDistance; }

void Camera::setWidth(float width) { this->width = width; }

float Camera::getWidth() const { return width; }

void Camera::setHeight(float height) { this->height = height; }

float Camera::getHeight() const { return height; }

float Camera::getAspectRatio() const { return width / height; }

void Camera::setSkybox(std::unique_ptr<Skybox> skybox) { this->skybox = std::move(skybox); }

Skybox* Camera::getSkybox() const { return skybox.get(); }

void Camera::setOrthographic(bool ortho) { this->orthographic = ortho; }

bool Camera::isOrthographic() const { return orthographic; }

void Camera::setOrthoSize(float size) { this->orthoSize = size; }

float Camera::getOrthoSize() const { return orthoSize; }

void Camera::setViewRect(float width, float height) {
    setWidth(width);
    setHeight(height);
}