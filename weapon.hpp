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
    virtual void fire(const glm::vec3& position, const glm::vec3& direction, InstanceID parentID, TeamID teamID) = 0;
    void playSound(const glm::vec3& position, const glm::vec3& direction);
    virtual bool hasEntity();
    std::shared_ptr<Entity> entity;
    std::string name;
    float range = 50.0f;
    float damage = 0.0f;
    std::vector<float> reload;
    virtual ~Weapon() = default;
    // I could use rtti, but this seems simpler
    virtual WeaponKind kindOf() = 0;
    std::string sound;
private:
};

class PlasmaCannon : public Weapon {
public:
    PlasmaCannon(std::shared_ptr<Entity> projectileEntity);
    virtual void fire(const glm::vec3& position, const glm::vec3& direction, InstanceID parentID, TeamID teamID);
    virtual bool hasEntity();
    virtual ~PlasmaCannon() = default;
    inline virtual WeaponKind kindOf() { return WeaponKind::PLASMA_CANNON; } 
private:
};

class Beam : public Weapon {
public:
    virtual void fire(const glm::vec3& position, const glm::vec3& target, InstanceID parentID, TeamID teamID);
    uint32_t color = 0xffffffff;
    static constexpr int framesToFire = Config::Net::ticksPerSecond * 0.5f;
    virtual ~Beam() = default;
    inline virtual WeaponKind kindOf() { return WeaponKind::BEAM; } 
};

class Guided : public PlasmaCannon {
public:
    Guided(std::shared_ptr<Entity> projectileEntity);
    virtual void fire(const glm::vec3& position, const glm::vec3& direction, InstanceID parentID, TeamID teamID);
    virtual ~Guided() = default;
    inline virtual WeaponKind kindOf() { return WeaponKind::GUIDED; }
};

class WeaponInstance {
public:
    WeaponInstance(Weapon *weapon, InstanceID parentID, TeamID teamID, const glm::vec3& offset);
    Weapon *instanceOf;
    WeaponKind kindOf;
    float timeSinceFired;
    uint reloadIndex = 0;
    glm::vec3 offset = glm::vec3(0.0f);

    InstanceID parentID;
    TeamID teamID;
    void fire(const glm::vec3& position, const glm::vec3& direction, const glm::mat3& transformation);
    int framesFired = 0;
    bool firing = false;
};
