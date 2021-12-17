#include "instance.hpp"
#include "econf.h"

#include <iostream>

void crcString(boost::crc_32_type& crc, const std::string& str) {
    crc.process_bytes(str.data(), str.length());
}

Instance::Instance() : id(42) { }

Instance::Instance(Entity *entity, InstanceID id) noexcept
: Instance(entity, nullptr, nullptr, id, true) { }

Instance::Instance(Entity* entity, EntityTexture* texture, SceneModelInfo* sceneModelInfo, InstanceID id, bool inPlay) noexcept
: id(id), inPlay(inPlay), entity(entity), texture(texture), sceneModelInfo(sceneModelInfo) {

    weapons.reserve(entity->weapons.size());
    for (auto& weapon : entity->weapons) {
        weapons.push_back(WeaponInstance(weapon, id));
    }

    if (inPlay && entity->isUnit) health = entity->maxHealth;
    if (inPlay && entity->isResource) resources = entity->resources;
}

InstanceUBO *Instance::getUBO(const glm::mat4& view, const glm::mat4& projView, const glm::mat4& view_1proj_1, float aspectRatio, float zMin, float zMax) {
    auto clipCoord = projView * glm::vec4(position.x, position.y, position.z, 1.0);
    if (renderAsIcon) {

        _state.model = view_1proj_1 * translate(glm::vec3(clipCoord.x / clipCoord.w, clipCoord.y / clipCoord.w, clipCoord.z / clipCoord.w))
            * scale(glm::vec3(Config::iconNDCSize / aspectRatio, Config::iconNDCSize, Config::iconNDCSize));

    } else {
        if (!entity->isProjectile)
            _state.model = translate(position) * glm::toMat4(heading);
        else {
            _state.model = translate(position) * glm::eulerAngleXYX(static_cast <float> (M_PI * rand()) / static_cast <float> (RAND_MAX),
                static_cast <float> (M_PI * rand()) / static_cast <float> (RAND_MAX), static_cast <float> (M_PI * rand()) / static_cast <float> (RAND_MAX));
        }
    }
    _state.normal = transpose(inverse(view * _state.model));
    // This math is wrong
    if (inPlay && !entity->isProjectile) _state.healthBarModel = view_1proj_1 * 
        translate(glm::vec3(clipCoord.x / clipCoord.w, clipCoord.y / clipCoord.w - 0.2,
        clipCoord.z / clipCoord.w - entity->boundingRadius / (sqrtf(cammeraDistance2 - 10.0f) * zMax) * 3)) *
        scale(glm::vec3(0.1f / aspectRatio, 0.01f, 0.1f));
    _state.health = health / entity->maxHealth;
    _state.resources = resources / entity->resources;

    return &_state;
}

void Instance::syncToAuthInstance(const Instance& other) {
    position = other.position;
    heading = other.heading;
    dP = other.dP;
    // dH = other.dH;
    commandList = other.commandList;
    health = other.health;
    state = other.state;
    uncompleted = other.uncompleted;
    resources = other.resources;
}

// NOTE: direction has to be normalized
bool Instance::rayIntersects(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    return intersectRaySphere(origin, direction, position, entity->boundingRadius * entity->boundingRadius, distance);
}

bool Instance::operator==(const Instance& other) const {
    return id == other.id;
}

bool Instance::operator==(uint32_t id) const {
    return this->id == id;
}

bool Instance::operator<(uint32_t id) const {
    return this->id < id;
}

void Instance::doCrc(boost::crc_32_type& crc) const {
    crcString(crc, entity->name);
    // TODO Finish this
}

// This is not really right
bool Instance::secondQueuedCommandRequiresMovement() const {
    if (commandList.size() < 2) return false;
    auto it = ++commandList.begin();
    switch (it->kind) {
        case CommandKind::MOVE:
        case CommandKind::TARGET_UNIT:
        case CommandKind::TARGET_LOCATION:
            return true;
        default:
            return false;
    }
}

bool Instance::canAttack() const {
    return !!weapons.size();
}