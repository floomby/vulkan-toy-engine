#include "instance.hpp"

#include <iostream>

Instance::Instance(Entity* entity, EntityTexture* texture, SceneModelInfo* sceneModelInfo, bool inPlay) noexcept
: id(0), inPlay(inPlay) {
    this->entity = entity;
    this->texture = texture;
    this->sceneModelInfo = sceneModelInfo;

    weapons.reserve(entity->weapons.size());
    for (auto& weapon : entity->weapons) {
        weapons.push_back(WeaponInstance(weapon));
    }
}

InstanceUBO *Instance::getUBO(const glm::mat4& view, const glm::mat4& projView, const glm::mat4& view_1proj_1, float aspectRatio, float zMin, float zMax) {
    auto clipCoord = projView * glm::vec4(position.x, position.y, position.z, 1.0);
    if (renderAsIcon) {

        _state.model = view_1proj_1 * translate(glm::vec3(clipCoord.x / clipCoord.w, clipCoord.y / clipCoord.w, clipCoord.z / clipCoord.w))
            * scale(glm::vec3(0.1f / aspectRatio, 0.1f, 0.1f));

    } else {
        _state.model = translate(position) * glm::toMat4(heading);
    }
    _state.normal = transpose(inverse(view * _state.model));
    if (inPlay && !entity->isProjectile) _state.healthBarModel = view_1proj_1 * 
        translate(glm::vec3(clipCoord.x / clipCoord.w, clipCoord.y / clipCoord.w - 0.2, clipCoord.z / clipCoord.w - 0.1)) *
        scale(glm::vec3(0.1f / aspectRatio, 0.01f, 0.1f));
    _state.health = health;

    return &_state;
}

void Instance::syncToAuthInstance(const Instance& other) {
    position = other.position;
    heading = other.heading;
    dP = other.dP;
    // dH = other.dH;
    commandList = other.commandList;
}

// NOTE: direction has to be normalized
bool Instance::rayIntersects(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    return intersectRaySphere(origin, direction, position, entity->boundingRadius * entity->boundingRadius, distance);
}

bool Instance::operator==(const Instance& other) {
    return id == other.id;
}

bool Instance::operator==(uint32_t id) {
    return this->id == id;
}

bool Instance::operator<(uint32_t id) {
    return this->id < id;
}