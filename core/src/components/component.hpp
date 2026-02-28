#ifndef COMPONENT_HPP
#define COMPONENT_HPP

class WorldObject;

class Component {
protected:
    WorldObject* owner = nullptr;

public:
    virtual ~Component() = default;
    
    void setOwner(WorldObject* obj) { owner = obj; }
    WorldObject* getOwner() const { return owner; }
};

#endif
