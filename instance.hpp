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
    glm::f32 highlight;
};

class Instance {
public:
    Instance(Entity* entity, InternalTexture* texture, SceneModelInfo* sceneModelInfo, int entityIndex) noexcept;
    // Instance& transform(glm::mat4 transformationMatrix) noexcept;

    UniformBufferObject *state(glm::mat4 view);
    SceneModelInfo* sceneModelInfo;
    int entityIndex;

    glm::vec3 position, heading;
    bool& highlight();

    bool intersects(glm::vec3 origin, glm::vec3 direction, float& distance) const;
    InternalTexture* texture;
private:
    bool _highlight;
    UniformBufferObject _state;
    Entity* entity;
};