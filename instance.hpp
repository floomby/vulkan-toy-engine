#pragma once

#include "entity.hpp"

class InternalTexture;

// Information about the model storage in vram
struct SceneModelInfo {
    size_t vertexOffset, indexOffset, indexCount;
};

class Instance {
public:
    Instance(Entity* entity, InternalTexture* texture, SceneModelInfo* sceneModelInfo) noexcept;
    Instance& Transform(glm::mat4 transformationMatrix) noexcept;

    // Is this the best way to hold this information?
    glm::mat4 coordinates;
private:
    Entity* entity;
    InternalTexture* texture;
    SceneModelInfo* sceneModelInfo;
};