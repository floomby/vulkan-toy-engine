#pragma once

#include "utilities.hpp"

#include <memory>

class Entity;

class Weapon {
public:
    virtual void fire(const glm::vec3& position, const glm::vec3& direction) = 0;
    virtual bool hasEntity();
    std::shared_ptr<Entity> entity;
    std::string name;
private:
};

enum class WeaponKind {
    PLASMA_CANNON,
    BEAM,
    GUIDED,
    COUNT
};

class PlasmaCannon : public Weapon {
public:
    PlasmaCannon(std::shared_ptr<Entity> projectileEntity);
    virtual void fire(const glm::vec3& position, const glm::vec3& direction);
    virtual bool hasEntity();
private:
};