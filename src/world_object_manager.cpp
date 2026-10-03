#include "world_object_manager.hpp"

WorldObject* WorldObjectManager::createObject() {
    auto obj = std::make_unique<WorldObject>();
    WorldObject* ptr = obj.get();
    objects.push_back(std::move(obj));
    return ptr;
}

const std::vector<std::unique_ptr<WorldObject>>& WorldObjectManager::getObjects() const {
    return objects;
}

void WorldObjectManager::clear() { objects.clear(); }