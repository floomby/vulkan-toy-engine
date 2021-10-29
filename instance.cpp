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

UniformBufferObject *Instance::state(const glm::mat4& view, const glm::vec3& cammeraPosition, const glm::vec3& cammeraStrafing,
    const glm::vec3& cammeraTarget, const glm::mat4& zRot2) {
    // I read that this is bad to do as it adds much to the memory cost of doing draws (it makes sense though)
    // probably fine for right now, but probably should be changed to something else
    if (renderAsIcon) {
        auto pointing = position - cammeraPosition;
        // auto strafing = normalize(cross(pointing, { 0.0f, 0.0f, 1.0f }));
        auto pointingLength = length(pointing);
        // badly behaved when both are vanishing near the extremes (I can pass in cammera.heading and make it better at the extremes)
        auto otherRot = glm::angleAxis(pointing.z / pointingLength - (float)M_PI_2, -cammeraStrafing);
        // auto fowarding = normalize(cross(strafing, { 0.0f, 0.0f, 1.0f }));
        // cammera.heading = normalize(cross(cammera.pointing, cammera.strafing));
        // auto zRot = glm::angleAxis(atan2f(pointing.y, pointing.x), glm::vec3({ 0.0f, 0.0f, 1.0f }));

        // law of cosines
        float a2_xy = sq(cammeraPosition.x - cammeraTarget.x) + sq(cammeraPosition.y - cammeraTarget.y);
        float a_xy = sqrtf(a2_xy);
        float b2_xy = sq(cammeraPosition.x - position.x) + sq(cammeraPosition.y - position.y);
        float b_xy = sqrtf(b2_xy);
        float c2_xy = sq(cammeraTarget.x - position.x) + sq(cammeraTarget.y - position.y);
        float axb_xy = (cammeraPosition.x - cammeraTarget.x) * (cammeraPosition.y - position.y) -
            (cammeraPosition.y - cammeraTarget.y) * (cammeraPosition.x - position.x);
        float theta = sgn(axb_xy) * acosf((a2_xy + b2_xy - c2_xy) / (2 * a_xy * b_xy));

        float a2_z = sq(cammeraPosition.z - cammeraTarget.z) + a2_xy;
        float a_z = sqrtf(a2_z);
        float b2_z = sq(cammeraPosition.z - position.z) + b2_xy;
        float b_z = sqrtf(b2_z);
        float c2_z = sq(cammeraTarget.z - position.z) + sq(a_xy - b_xy);
        float axb_z = (cammeraPosition.z - cammeraTarget.z) * b2_xy - (cammeraPosition.z - position.z) * a2_xy;
        float phi = sgn(axb_z) * acosf((a2_z + b2_z - c2_z) / (2 * a_z * b_z));

        // Getting real close to the cammera there is still a shear factor error, but this is for rendering icons of things far away
        // so I don't know if fixing that is worth it

        glm::mat4 shear = {
            1.0f, asinf(phi * theta), 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };

        float scaleFactor = pointingLength / 16.0f;

        _state.model = translate(position) * scale(glm::vec3({ scaleFactor, scaleFactor, scaleFactor })) * toMat4(-otherRot) * zRot2 * shear;
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