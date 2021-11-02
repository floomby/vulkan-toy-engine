#include "instance.hpp"

Instance::Instance(Entity* entity, InternalTexture* texture, SceneModelInfo* sceneModelInfo, int entityIndex) noexcept
: id(idCounter++) {
    this->entityIndex = entityIndex;
    // WARNING: This likly will cause problems since the entities are moved by stl code sometimes (change me to just use the index)
    this->entity = entity; // !!!!!!!!!! This is problematic
    this->texture = texture;
    this->sceneModelInfo = sceneModelInfo;
    highlight = false;
};

// Instance& Instance::transform(glm::mat4 transformationMatrix) noexcept {
//     state.model *= transformationMatrix;
//     return *this;
// };

#include <iostream>

UniformBufferObject *Instance::state(const glm::mat4& view, const glm::mat4& proj, const glm::mat4& view_1proj_1, float aspectRatio, float zMin, float zMax) {
    // I read that this is bad to do as it adds much to the memory cost of doing draws (it makes sense though)
    // probably fine for right now, but probably should be changed to something else
    if (renderAsIcon) {
        auto clipCoord = proj * view * glm::vec4(position.x, position.y, position.z, 1.0);

        _state.model = view_1proj_1 * translate(glm::vec3(clipCoord.x / clipCoord.w, clipCoord.y / clipCoord.w, clipCoord.z / clipCoord.w)) * scale(glm::vec3(0.1f / aspectRatio, 0.1f, 0.1f));
    } else {
        _state.model = translate(position) * glm::eulerAngleYXZ(heading.y, heading.x, heading.z);
    }
    // position.z -= 0.01f;
    _state.normal = transpose(inverse(view * _state.model));
    return &_state;
}

// NOTE: direction has to be normalized
bool Instance::intersects(glm::vec3 origin, glm::vec3 direction, float& distance) const {
    return intersectRaySphere(origin, direction, position, entity->boundingRadius * entity->boundingRadius, distance);
}