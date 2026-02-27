#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "color.hpp"
#include "vector3.hpp"
#include <cstdint>

enum class LightType : uint8_t { DIRECTIONAL = 0, POINT = 1, SPOT = 2 };

struct Light {
    LightType type;
    Vector3 direction;
    ColorRGBA color;
    float intensity;
    // float position[3];
};

#endif
