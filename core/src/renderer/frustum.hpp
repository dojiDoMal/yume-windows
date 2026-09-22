#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include "../math.hpp"
#include "../vector3.hpp"
#include <glm/glm.hpp>

// A view frustum represented as its 6 clipping planes, extracted from a
// combined view-projection matrix (Gribb & Hartmann method). Each plane is
// stored as (a, b, c, d) where a*x + b*y + c*z + d = 0, with the normal
// (a, b, c) pointing toward the inside of the frustum.
class Frustum {
  public:
    enum PlaneID { LEFT = 0, RIGHT, BOTTOM, TOP, NEAR, FAR, PLANE_COUNT };

    // Build the frustum from a view-projection matrix (projection * view).
    void fromViewProjection(const glm::mat4& m) {
        // Rows of the matrix. glm is column-major, so m[col][row]; we read
        // rows as (m[0][r], m[1][r], m[2][r], m[3][r]).
        const glm::vec4 row0(m[0][0], m[1][0], m[2][0], m[3][0]);
        const glm::vec4 row1(m[0][1], m[1][1], m[2][1], m[3][1]);
        const glm::vec4 row2(m[0][2], m[1][2], m[2][2], m[3][2]);
        const glm::vec4 row3(m[0][3], m[1][3], m[2][3], m[3][3]);

        planes[LEFT] = row3 + row0;
        planes[RIGHT] = row3 - row0;
        planes[BOTTOM] = row3 + row1;
        planes[TOP] = row3 - row1;
        planes[NEAR] = row3 + row2;
        planes[FAR] = row3 - row2;

        for (int i = 0; i < PLANE_COUNT; ++i)
            normalizePlane(planes[i]);
    }

    // Returns true if the world-space sphere is at least partially inside the
    // frustum. A sphere is fully outside only if it lies entirely on the
    // negative side of any single plane.
    bool intersectsSphere(const Vector3& center, float radius) const {
        for (int i = 0; i < PLANE_COUNT; ++i) {
            float dist = planes[i].x * center.x + planes[i].y * center.y + planes[i].z * center.z +
                         planes[i].w;
            if (dist < -radius)
                return false; // completely behind this plane -> outside
        }
        return true;
    }

  private:
    glm::vec4 planes[PLANE_COUNT];

    static void normalizePlane(glm::vec4& p) {
        float len = Yume::Math::length({p.x, p.y, p.z});
        if (len > 0.0f)
            p /= len;
    }
};

#endif // FRUSTUM_HPP
