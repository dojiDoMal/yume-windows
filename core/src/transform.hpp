#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "matrix4.hpp"
#include "vector3.hpp"

class Transform {
  private:
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    mutable Matrix4 cachedMatrix{1.0f};
    mutable bool dirty = true;

  public:
    Matrix4 getModelMatrix() const;

    Vector3 getPosition() const;
    void setPosition(const Vector3& pos);

    Vector3 getRotation() const;
    void setRotation(const Vector3& rot);

    Vector3 getScale() const;
    void setScale(const Vector3& scl);
};

#endif