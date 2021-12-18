#pragma once

#include "utilities.hpp"
#include "econf.h"

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

enum class WeaponKind {
    PLASMA_CANNON,
    BEAM,
    GUIDED
};

class Weapon {
public:
    virtual void fire(const glm::vec3& position, const glm::vec3& direction, InstanceID parentID) = 0;
    virtual bool hasEntity();
    std::shared_ptr<Entity> entity;
    std::string name;
    float range = 50.0f;
    float damage = 0.0f;
    std::vector<float> reload;
    virtual ~Weapon() = default;
    // I could use rtti, but this seems simpler
    virtual WeaponKind kindOf() = 0;
private:
};

class PlasmaCannon : public Weapon {
public:
    PlasmaCannon(std::shared_ptr<Entity> projectileEntity);
    virtual void fire(const glm::vec3& position, const glm::vec3& direction, InstanceID parentID);
    virtual bool hasEntity();
    virtual ~PlasmaCannon() = default;
    inline virtual WeaponKind kindOf() { return WeaponKind::PLASMA_CANNON; } 
private:
};

class Beam : public Weapon {
public:
    virtual void fire(const glm::vec3& position, const glm::vec3& target, InstanceID parentID);
    uint32_t color = 0xffffffff;
    static constexpr int framesToFire = Config::Net::ticksPerSecond * 0.5f;
    virtual ~Beam() = default;
    inline virtual WeaponKind kindOf() { return WeaponKind::BEAM; } 
};

class WeaponInstance {
public:
    WeaponInstance(Weapon *weapon, InstanceID parentID);
    Weapon *instanceOf;
    WeaponKind kindOf;
    // Target target;
    // void aquireTarget(/* needs some argements*/);
    float timeSinceFired;
    uint reloadIndex;

    uint32_t parentID;
    void fire(const glm::vec3& position, const glm::vec3& direction);
    // glm::vec3 realativePosition;
    int framesFired = 0;
    bool firing = false;
};
