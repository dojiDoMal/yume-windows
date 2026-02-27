#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "vector3.hpp"
#include <glm/glm.hpp>

class Transform {
  private:
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;

  public:
    glm::mat4 getModelMatrix() const;

    Vector3 getPosition() const;
    void setPosition(const Vector3& pos);

    Vector3 getRotation() const;
    void setRotation(const Vector3& rot);

    Vector3 getScale() const;
    void setScale(const Vector3& scl);
};

#endif