#pragma once

#include <coroutine>
#include <list>
#include "entity.hpp"

typedef uint32_t InstanceID;

enum class CommandKind {
    MOVE,
    ATTACK,
    STOP,
    COUNT,
    //.....
};

struct CommandData {
    glm::vec3 dest;
    uint32_t id;
};

struct Command {
    CommandKind kind;
    InstanceID id;
    CommandData data;
};

// typedef std::list<std::pair<Command, uint32_t>> CommandList;

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
    Instance(Entity* entity, InternalTexture* texture, SceneModelInfo* sceneModelInfo, bool inPlay) noexcept;
    void syncToAuthInstance(const Instance& other);

    SceneModelInfo* sceneModelInfo;

    UniformBufferObject *state(const glm::mat4& view, const glm::mat4& projView, const glm::mat4& view_1proj_1, float aspectRatio, float zMin, float zMax);

    bool intersects(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;
    InternalTexture* texture;

    bool renderAsIcon = false;
    float cammeraDistance2;
    bool highlight, rendered;

    bool inPlay;
    bool orphaned = false;
    InstanceID id;
    Entity *entity;
    int team = 0; // default to team 0 which is gaia

    // This is the stuff that needs to get synced
    glm::vec3 position, dP = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat heading;
    std::list<Command> commandList;
private:
    UniformBufferObject _state;
};