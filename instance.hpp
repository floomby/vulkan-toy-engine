#pragma once

#include "entity.hpp"

class InternalTexture;

// Information about the model storage in vram
struct SceneModelInfo {
    size_t vertexOffset, indexOffset, indexCount;
};

struct UniformBufferObject {
    glm::mat4 model;
};

class Instance {
public:
    Instance(Entity* entity, InternalTexture* texture, SceneModelInfo* sceneModelInfo) noexcept;
    Instance& Transform(glm::mat4 transformationMatrix) noexcept;

    UniformBufferObject state;
    SceneModelInfo* sceneModelInfo;
private:
    Entity* entity;
    InternalTexture* texture;
};