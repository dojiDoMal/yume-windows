#include "light.hpp"

void Light::setType(LightType t) { type = t; }

LightType Light::getType() const { return type; }

void Light::setDirection(const Vector3& dir) { direction = dir; }

const Vector3& Light::getDirection() const { return direction; }

void Light::setColor(const ColorRGBA& c) { color = c; }

const ColorRGBA& Light::getColor() const { return color; }

void Light::setIntensity(float i) { intensity = i; }

float Light::getIntensity() const { return intensity; }