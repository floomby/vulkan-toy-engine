#include "instance.hpp"

Instance::Instance(Entity* entity, InternalTexture* texture, SceneModelInfo* sceneModelInfo, int entityIndex) noexcept {
    this->entityIndex = entityIndex;
    // WARNING: This likly will cause problems since the entities are moved by stl code sometimes (change me to just use the index)
    this->entity = entity;
    this->texture = texture;
    this->sceneModelInfo = sceneModelInfo;
    _state.highlight = 0.0;
    _highlight = false;
};

// Instance& Instance::transform(glm::mat4 transformationMatrix) noexcept {
//     state.model *= transformationMatrix;
//     return *this;
// };

#include <iostream>

UniformBufferObject *Instance::state(glm::mat4 view, glm::vec3 cammeraPosition) {
    // I read that this is bad to do as it adds much to the memory cost of doing draws (it makes sense though)
    // probably fine for right now, but probably should be changed to something else
    if (renderAsIcon) {
        auto pointing = position - cammeraPosition;
        auto strafing = normalize(cross(pointing, { 0.0f, 0.0f, 1.0f }));
        auto pointingLength = length(pointing);
        // badly behaved when both are vanishing near the extremes (I can pass in cammera.heading and make it better at the extremes)
        auto zRot = glm::angleAxis(atan2f(pointing.y, pointing.x), glm::vec3({ 0.0f, 0.0f, 1.0f }));
        auto otherRot = glm::angleAxis(pointing.z / pointingLength - (float)M_PI_2, strafing);
        // auto fowarding = normalize(cross(strafing, { 0.0f, 0.0f, 1.0f }));
        // cammera.heading = normalize(cross(cammera.pointing, cammera.strafing));
        _state.model = translate(position) * toMat4(-otherRot) * toMat4(zRot);
    } else { 
        _state.model = translate(position) * glm::eulerAngleYXZ(heading.y, heading.x, heading.z);
    }
    // position.z -= 0.01f;
    _state.normal = transpose(inverse(view * _state.model));
    _state.highlight = _highlight ? 1.0 : 0.0;
    return &_state;
}

bool& Instance::highlight() {
    return _highlight;
}

// NOTE: direction has to be normalized
bool Instance::intersects(glm::vec3 origin, glm::vec3 direction, float& distance) const {
    return intersectRaySphere(origin, direction, position, entity->boundingRadius * entity->boundingRadius, distance);
}