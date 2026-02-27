#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "vector3.hpp"
#include <cstdint>

enum class LightType : uint8_t { DIRECTIONAL = 0, POINT = 1, SPOT = 2 };

struct Light {
    LightType type;
    Vector3 direction;
    // float position[3];
};

#endif
