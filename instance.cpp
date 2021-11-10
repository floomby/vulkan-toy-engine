#include "instance.hpp"

#include <iostream>

Instance::Instance(Entity* entity, InternalTexture* texture, SceneModelInfo* sceneModelInfo, bool inPlay) noexcept
: id(0), inPlay(inPlay) {
    // WARNING: This likly will cause problems since the entities are moved by stl code sometimes (change me to just use the index)
    this->entity = entity; // !!!!!!!!!! This is problematic
    this->texture = texture;
    this->sceneModelInfo = sceneModelInfo;
    highlight = false;
    commandList.push_back({ CommandKind::MOVE, id, {{ 0.0f, 5.0f, 0.0f }, id }});
    // commandList.push_back({ CommandKind::MOVE, id, {{ 5.0f, 0.0f, 0.0f }, -1 }});
};

UniformBufferObject *Instance::state(const glm::mat4& view, const glm::mat4& projView, const glm::mat4& view_1proj_1, float aspectRatio, float zMin, float zMax) {
    // I read that this is bad to do as it adds much to the memory cost of doing draws (it makes sense though)
    // probably fine for right now, but probably should be changed to something else
    if (renderAsIcon) {
        auto clipCoord = projView * glm::vec4(position.x, position.y, position.z, 1.0);

        _state.model = view_1proj_1 * translate(glm::vec3(clipCoord.x / clipCoord.w, clipCoord.y / clipCoord.w, clipCoord.z / clipCoord.w))
            * scale(glm::vec3(0.1f / aspectRatio, 0.1f, 0.1f));
    } else {
        _state.model = translate(position) * glm::toMat4(heading);
    }
    // position.z -= 0.01f;
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
bool Instance::intersects(glm::vec3 origin, glm::vec3 direction, float& distance) const {
    return intersectRaySphere(origin, direction, position, entity->boundingRadius * entity->boundingRadius, distance);
}