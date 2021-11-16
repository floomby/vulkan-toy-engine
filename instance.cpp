#include "instance.hpp"

#include <iostream>

Instance::Instance(Entity* entity, InternalTexture* texture, SceneModelInfo* sceneModelInfo, bool inPlay) noexcept
: id(0), inPlay(inPlay) {
    this->entity = entity;
    this->texture = texture;
    this->sceneModelInfo = sceneModelInfo;

    weapons.reserve(entity->weapons.size());
    for (auto& weapon : entity->weapons) {
        weapons.push_back(WeaponInstance(weapon));
    }
}

UniformBufferObject *Instance::getUBO(const glm::mat4& view, const glm::mat4& projView, const glm::mat4& view_1proj_1, float aspectRatio, float zMin, float zMax) {
    if (renderAsIcon) {
        auto clipCoord = projView * glm::vec4(position.x, position.y, position.z, 1.0);

        _state.model = view_1proj_1 * translate(glm::vec3(clipCoord.x / clipCoord.w, clipCoord.y / clipCoord.w, clipCoord.z / clipCoord.w))
            * scale(glm::vec3(0.1f / aspectRatio, 0.1f, 0.1f));
    } else {
        _state.model = translate(position) * glm::toMat4(heading);
    }
    _state.normal = transpose(inverse(view * _state.model));
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