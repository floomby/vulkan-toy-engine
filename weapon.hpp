#pragma once

#include "utilities.hpp"

#include <memory>

class Entity;
class Instance;

class Target {
public:
    Target();
    Target(const glm::vec3& location);
    Target(InstanceID targetID);
    bool isLocation = false;
    bool isUnit = false;
    glm::vec3 location;
    InstanceID targetID;
};

class Weapon {
public:
    virtual void fire(const glm::vec3& position, const glm::vec3& direction, InstanceID parentID) = 0;
    virtual bool hasEntity();
    std::shared_ptr<Entity> entity;
    std::string name;
    float range;
    float damage;
private:
};

enum class WeaponKind {
    PLASMA_CANNON,
    BEAM,
    GUIDED
};

class PlasmaCannon : public Weapon {
public:
    PlasmaCannon(std::shared_ptr<Entity> projectileEntity);
    virtual void fire(const glm::vec3& position, const glm::vec3& direction, InstanceID parentID);
    virtual bool hasEntity();
private:
};

class WeaponInstance {
public:
    WeaponInstance(Weapon *weapon, InstanceID parentID);
    Weapon *instanceOf;
    // Target target;
    // void aquireTarget(/* needs some argements*/);
    float timeSinceFired;

    uint32_t parentID;
    void fire(const glm::vec3& position);
    // glm::vec3 realativePosition;
};