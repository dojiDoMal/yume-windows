#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "color.hpp"
#include "skybox.hpp"
#include "vector3.hpp"

class Camera {
  private:
    ColorRGBA backgroundColor = {0.2f, 0.3f, 0.3f, 1.0f};
    Vector3 position = {0.0f, 2.0f, 2.0f};
    Vector3 target = {0.0f, 0.0f, 0.0f};
    std::unique_ptr<Skybox> skybox;
    float fov = 45.0f;
    float nearDistance = 0.1f;
    float farDistance = 100.0f;
    float width = 800.0f;
    float height = 600.0f;
    bool orthographic = false;
    float orthoSize = 1.0f;

  public:
    Camera() = default;
    ~Camera() = default;

    const Vector3& getPosition() const;
    void setPosition(const Vector3& position);
    const Vector3& getTarget() const;
    void setTarget(const Vector3& target);
    ColorRGBA& getBackgroundColor();
    void setBackgroundColor(const ColorRGBA& color);
    void setFov(float fov);
    float getFov() const;
    void setNearDistance(float nearDistance);
    float getNearDistance() const;
    void setFarDistance(float farDistance);
    float getFarDistance() const;
    void setWidth(float width);
    float getWidth() const;
    void setHeight(float height);
    float getHeight() const;
    float getAspectRatio() const;
    void setOrthoSize(float size);
    float getOrthoSize() const;
    
    void setSkybox(std::unique_ptr<Skybox> skybox);
    Skybox* getSkybox() const;
    void setOrthographic(bool ortho);
    bool isOrthographic() const;
    void setViewRect(float width, float height);
};

#endif // CAMERA_HPP
