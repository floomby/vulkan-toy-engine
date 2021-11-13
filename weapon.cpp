#include "api.hpp"
#include "weapon.hpp"

bool Weapon::hasEntity() {
    return true;
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