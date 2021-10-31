#pragma once

#include "entity.hpp"

typedef uint32_t InstanceID;

class InternalTexture;

// Information about the model storage in vram
struct SceneModelInfo {
    size_t vertexOffset, indexOffset, indexCount;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 normal;
};

class Instance {
public:
    inline static InstanceID idCounter = 0;
    Instance(Entity* entity, InternalTexture* texture, SceneModelInfo* sceneModelInfo, int entityIndex) noexcept;
    // Instance& transform(glm::mat4 transformationMatrix) noexcept;

    UniformBufferObject *state(const glm::mat4& view, const glm::vec3& cammeraPosition, const glm::vec3& cammeraStrafing,
        const glm::vec3& cammeraTarget, const glm::mat4& zRot, const float skewCorrectionFactor);
    SceneModelInfo* sceneModelInfo;
    int entityIndex;

    glm::vec3 position, heading;

    bool intersects(glm::vec3 origin, glm::vec3 direction, float& distance) const;
    InternalTexture* texture;

    bool renderAsIcon = false;
    float cammeraDistance2;
    bool highlight;

    InstanceID id;
private:
    UniformBufferObject _state;
    Entity* entity;
};