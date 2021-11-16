#include "api.hpp"
#include "weapon.hpp"
#include "net.hpp"

bool Weapon::hasEntity() {
    return false;
}

PlasmaCannon::PlasmaCannon(std::shared_ptr<Entity> projectileEntity) {
    entity = projectileEntity;
}

void PlasmaCannon::fire(const glm::vec3& position, const glm::vec3& normedDirection) {
    Api::eng_createBallisticProjectile(entity.get(), position, normedDirection);
}

bool PlasmaCannon::hasEntity() {
    return true;
}

Target::Target() {}

Target::Target(const glm::vec3& location)
: isLocation(true), location(location) {}

Target::Target(Instance *instance)
: isUnit(true), instance(instance) {}

WeaponInstance::WeaponInstance(Weapon *instanceOf)
: instanceOf(instanceOf), timeSinceFired(0.0f) {}

void WeaponInstance::fire(const glm::vec3& position) {
    if (timeSinceFired > 1.0f) {
        instanceOf->fire(position, { 0.0f, 0.0f, 1.0f });
        timeSinceFired = 0.0f;
    }
    timeSinceFired += Net::secondsPerTick;
}