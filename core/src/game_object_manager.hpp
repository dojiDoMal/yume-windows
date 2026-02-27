#ifndef GAME_OBJECT_MANAGER_HPP
#define GAME_OBJECT_MANAGER_HPP

#include "game_object.hpp"

class GameObjectManager {
    std::vector<GameObject*> objects;

  public:
    ~GameObjectManager() {
        for (size_t i = 0; i < objects.size(); ++i) {
            delete objects[i];
        }
    }
    void add(GameObject* obj) { objects.push_back(obj); }
    std::vector<GameObject*>& get() { return objects; }
};

#endif