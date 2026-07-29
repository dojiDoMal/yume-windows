#ifndef LOD_GROUP_HPP
#define LOD_GROUP_HPP

#include "../mesh.hpp"
#include "component.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

struct LodLevel {
    std::shared_ptr<Mesh> mesh;
    float screenSpaceThreshold;
};

class LodGroup : public Component {
  private:
    std::vector<LodLevel> levels;
    int activeLevelIndex = 0;

    float computeScreenSpacePercentage(const glm::vec3& objPos, float radius,
                                       const glm::vec3& camPos, float fovRad,
                                       float viewportHeight) const {
        float dist = glm::length(objPos - camPos);
        if (dist < 0.0001f)
            return 1.0f;
        float projectedSize = (radius / (dist * std::tan(fovRad * 0.5f)));
        return projectedSize; // já é fração da altura da viewport
    }

  public:
    void addLevel(std::shared_ptr<Mesh> mesh, float threshold) {
        levels.push_back({std::move(mesh), threshold});
    }

    // Retorna true se o objeto deve ser renderizado (não culled)
    bool update(const glm::vec3& objPos, float boundingRadius, const glm::vec3& camPos,
                float fovDeg, float viewportHeight) {
        float fovRad = glm::radians(fovDeg);
        float ssp =
            computeScreenSpacePercentage(objPos, boundingRadius, camPos, fovRad, viewportHeight);

        // Percorre do LOD0 (maior threshold) ao último
        for (int i = 0; i < (int)levels.size(); i++) {
            if (ssp >= levels[i].screenSpaceThreshold) {
                activeLevelIndex = i;
                return true;
            }
        }
        // Culled
        return false;
    }

    Mesh* getActiveMesh() const {
        if (activeLevelIndex < (int)levels.size())
            return levels[activeLevelIndex].mesh.get();
        return nullptr;
    }

    std::shared_ptr<Mesh> getActiveMeshShared() const {
        if (activeLevelIndex < (int)levels.size())
            return levels[activeLevelIndex].mesh;
        return nullptr;
    }

    int getLevelCount() const { return (int)levels.size(); }
};

#endif
