#include "api.hpp"
#include "weapon.hpp"
#include "net.hpp"
#include "base.hpp"

bool Weapon::hasEntity() {
    return false;
}

void Weapon::playSound(const glm::vec3& position, const glm::vec3& direction) {
    if (!Api::context->headless && !sound.empty()) {
        Api::eng_playSound3d(sound.c_str(), position, direction);
    }
}

std::ostream& operator<<(std::ostream& os, const Weapon& weapon) {
    os << weapon.name;
    return os;
}


PlasmaCannon::PlasmaCannon(std::shared_ptr<Entity> projectileEntity) {
    entity = projectileEntity;
    entity->weapon = this;
}

void PlasmaCannon::fire(const glm::vec3& position, const glm::vec3& normedDirection, InstanceID parentID, TeamID teamID) {
    Api::eng_createBallisticProjectile(entity.get(), position, normedDirection, parentID);
}

bool PlasmaCannon::hasEntity() {
    return true;
}

void Beam::fire(const glm::vec3& position, const glm::vec3& target, InstanceID parentID, TeamID teamID) {
    Api::eng_createBeam(color, damage, position, target, parentID);
}

Guided::Guided(std::shared_ptr<Entity> projectileEntity) : PlasmaCannon(projectileEntity) {}

void Guided::fire(const glm::vec3& position, const glm::vec3& normedDirection, InstanceID parentID, TeamID teamID) {
    Api::eng_createGuidedProjectile(entity.get(), position, normedDirection, parentID, teamID);
}

Target::Target() {}

Target::Target(const glm::vec3& location)
: isLocation(true), location(location) {}

Target::Target(InstanceID targetID)
: isUnit(true), targetID(targetID) {}

WeaponInstance::WeaponInstance(Weapon *instanceOf, InstanceID parentID, TeamID teamID, const glm::vec3& offset)
: instanceOf(instanceOf), timeSinceFired(0.0f), parentID(parentID), teamID(teamID), kindOf(instanceOf->kindOf()), offset(offset) {}

void WeaponInstance::fire(const glm::vec3& position, const glm::vec3& direction, const glm::mat3& transformation) {
    if (timeSinceFired > instanceOf->reload[reloadIndex]) {
        if (kindOf != WeaponKind::BEAM) {
            instanceOf->fire(position + transformation * offset, direction, parentID, teamID);
        }
        instanceOf->playSound(position + transformation * offset, direction);
        timeSinceFired = 0.0f;
        reloadIndex = (reloadIndex + 1) % instanceOf->reload.size();
        firing = true;
        framesFired = 0;
    }
    if (kindOf == WeaponKind::BEAM && firing) {
        if (framesFired++ < Beam::framesToFire) {
            instanceOf->fire(position + transformation * offset, direction, parentID, teamID);
        } else firing = false;
    }
    timeSinceFired += Config::Net::secondsPerTick;
}