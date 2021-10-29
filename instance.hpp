#pragma once

#include "entity.hpp"

class InternalTexture;

// Information about the model storage in vram
struct SceneModelInfo {
    size_t vertexOffset, indexOffset, indexCount;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 normal;
    // convert this to a uint32 for combining flags
    glm::f32 highlight;
};

class Instance {
public:
    Instance(Entity* entity, InternalTexture* texture, SceneModelInfo* sceneModelInfo, int entityIndex) noexcept;
    // Instance& transform(glm::mat4 transformationMatrix) noexcept;

    UniformBufferObject *state(const glm::mat4& view, const glm::vec3& cammeraPosition, const glm::vec3& cammeraStrafing, const glm::vec3& cammeraTarget, const glm::mat4& zRot);
    SceneModelInfo* sceneModelInfo;
    int entityIndex;

    glm::vec3 position, heading;
    bool& highlight();

    bool intersects(glm::vec3 origin, glm::vec3 direction, float& distance) const;
    InternalTexture* texture;
    bool renderAsIcon = false;
private:
    bool _highlight;
    UniformBufferObject _state;
    Entity* entity;
};