#pragma once

// One of the big things about this code is that this class is storing information pertinent to rendering as well
// as the information about the game state of this object, I have no idea if this is good or not (I suspect it is the later)

#include <coroutine>
#include <list>
#include "entity.hpp"
#include "team.hpp"

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

class EntityTexture;

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
    Instance(Entity* entity, EntityTexture* texture, SceneModelInfo* sceneModelInfo, bool inPlay) noexcept;
    void syncToAuthInstance(const Instance& other);

    SceneModelInfo* sceneModelInfo;

    UniformBufferObject *getUBO(const glm::mat4& view, const glm::mat4& projView, const glm::mat4& view_1proj_1, float aspectRatio, float zMin, float zMax);

    bool rayIntersects(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;
    EntityTexture* texture;

    bool renderAsIcon = false;
    float cammeraDistance2;
    bool highlight = false, rendered;

    bool inPlay;
    bool orphaned = false;
    InstanceID id;
    Entity *entity;
    TeamID team = 0; // default to team 0 which is gaia

    // This is the stuff that needs to get synced
    glm::vec3 position, dP = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat heading;
    std::list<Command> commandList;

    Target target;
    std::vector<WeaponInstance> weapons;
private:
    UniformBufferObject _state;
};