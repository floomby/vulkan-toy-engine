#pragma once

// One of the big things about this code is that this class is storing information pertinent to rendering as well
// as the information about the game state of this object, I have no idea if this is good or not (I suspect it is the later)

#include <boost/crc.hpp>
#include <coroutine>
#include <list>

#include "team.hpp"
#include "utilities.hpp"
#include "enum_helper.hpp"

void crcString(boost::crc_32_type& crc, const std::string& str);

enum class CommandKind {
    ATTACK,
    MOVE,
    STOP,
    CREATE,
    DESTROY,
    //.....
};

struct CommandData {
    glm::vec3 dest;
    glm::quat heading;
    uint32_t id;
};

struct Command {
    CommandKind kind;
    InstanceID id;
    CommandData data;
    InsertionMode mode;
};

static const std::vector<std::string> CommandKinds = enumNames2<CommandKind>();
static const std::vector<std::string> InsertionModes = enumNames2<InsertionMode>();

namespace std {
    inline ostream& operator<<(ostream& os, const Command& command) {
        os << "Command kind: " << CommandKinds[static_cast<int>(command.kind)] << " id: " << command.id << " mode: "
            << InsertionModes[static_cast<int>(command.mode)] << " " << command.data.dest << " - " << command.data.heading << " other id: " << command.data.id;
        return os;
    }
}

#include "entity.hpp"
#include "weapon.hpp"

class EntityTexture;

// Information about the model storage in vram
struct SceneModelInfo {
    size_t vertexOffset, indexOffset, indexCount;
};

struct InstanceUBO {
    glm::mat4 model;
    glm::mat4 normal;
    glm::mat4 healthBarModel;
    float health;
};

enum class IEngage {
    ENGAGE,
    AVOID
};

struct InstanceState {
    IEngage engageKind;
};

class Entity;

class Instance {
public:
    Instance();
    Instance(Entity* entity, InstanceID id) noexcept;
    Instance(Entity* entity, EntityTexture* texture, SceneModelInfo* sceneModelInfo, InstanceID id, bool inPlay) noexcept;
    void syncToAuthInstance(const Instance& other);

    SceneModelInfo* sceneModelInfo;

    InstanceUBO *getUBO(const glm::mat4& view, const glm::mat4& projView, const glm::mat4& view_1proj_1, float aspectRatio, float zMin, float zMax);

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
    float health = 0.3f;

    // This is the stuff that needs to get synced
    glm::vec3 position, dP = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat heading;
    std::list<Command> commandList;

    Target target;
    std::vector<WeaponInstance> weapons;

    InstanceState state { IEngage::ENGAGE };

    InstanceID parentID = 0;

    bool operator==(const Instance& other);
    bool operator==(uint32_t id);
    bool operator<(uint32_t id);

    void doCrc(boost::crc_32_type& crc) const;
private:
    InstanceUBO _state;
};