#ifndef WORLD_OBJECT_MANAGER_HPP
#define WORLD_OBJECT_MANAGER_HPP

#include "world_object.hpp"
#include <memory>
#include <vector>

class WorldObjectManager {
  private:
    std::vector<std::unique_ptr<WorldObject>> objects;

  public:
    WorldObjectManager() = default;

    WorldObject* createObject();

    const std::vector<std::unique_ptr<WorldObject>>& getObjects() const;

    void clear();
};

#endif
