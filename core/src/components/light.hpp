#ifndef LIGHT_COMPONENT_HPP
#define LIGHT_COMPONENT_HPP

#include "../color.hpp"
#include "../vector3.hpp"
#include "component.hpp"
#include <cstdint>

enum class LightType : uint8_t { DIRECTIONAL = 0, POINT = 1, SPOT = 2 };

class Light : public Component {
  private:
    LightType type = LightType::DIRECTIONAL;
    Vector3 direction = {0.0f, -1.0f, 0.0f};
    ColorRGBA color = {1.0f, 1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;

  public:
    Light() = default;

    void setType(LightType t);
    LightType getType() const;
    void setDirection(const Vector3& dir);
    const Vector3& getDirection() const;
    void setColor(const ColorRGBA& c);
    const ColorRGBA& getColor() const;
    void setIntensity(float i);
    float getIntensity() const;
};

#endif
