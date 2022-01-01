#pragma once

// One of the big things about this code is that this class is storing information pertinent to rendering as well
// as the information about the game state of this object, I have no idea if this is good or not (I suspect it is the later)

#include <boost/crc.hpp>
#include <coroutine>
#include <list>
#include <map>

#include "team.hpp"
#include "utilities.hpp"
#include "enum_helper.hpp"

void crcString(boost::crc_32_type& crc, const std::string& str);

enum class CommandKind {
    // ATTACK,
    MOVE,
    STOP,
    CREATE,
    DESTROY,
    TARGET_LOCATION,
    TARGET_UNIT,
    BUILD,
    STATE,
    INTRINSIC_STATE,
    //.....
};

const size_t CommandDataBufSize = 256;

struct CommandData {
    glm::vec3 dest;
    glm::quat heading;
    union {
        uint32_t id;
        uint32_t value;
    };
    char buf[CommandDataBufSize];
    IntrinicStates intrinicState;
};

struct Command {
    CommandKind kind;
    InstanceID id;
    CommandData data;
    InsertionMode mode;
    bool userDerived;
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
    float resources;
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
    Instance(Entity* entity, InstanceID id, TeamID team) noexcept;
    Instance(Entity* entity, EntityTexture* texture, SceneModelInfo* sceneModelInfo, InstanceID id, TeamID team, bool inPlay) noexcept;
    void syncToAuthInstance(const Instance& other);

    SceneModelInfo* sceneModelInfo = nullptr;

    InstanceUBO *getUBO(const glm::mat4& view, const glm::mat4& projView, const glm::mat4& view_1proj_1, float aspectRatio, float zMin, float zMax);

    bool rayIntersects(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;
    EntityTexture* texture = nullptr;

    bool secondQueuedCommandRequiresMovement() const;

    bool renderAsIcon = false;
    float cammeraDistance2; // , cammeraDistance;
    bool highlight = false, rendered = false;

    bool valid = true;
    bool dontDelete = false;
    bool inPlay = false;
    bool orphaned = false;
    bool isPlacement = false;

    std::array<int, IS_COUNT> intrinicStates = { 0, 1 };

    std::array<bool, Config::maxTeams> outOfFog, outOfRadar;

    InstanceID parentID = 0;

    // Game logic properties
    InstanceID id;
    InstanceID whatBuilding = 0;
    Entity *entity = nullptr;
    TeamID team = 0; // default to team 0 which is gaia
    double health = 1.0;
    double resources = 0.0;
    bool hasCollision = true;

    // This is the stuff that needs to get synced
    glm::vec3 position, dP = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat heading = { 1.0, 0.0, 0.0, 0.0 };
    std::list<Command> commandList;

    Target target;
    std::vector<WeaponInstance> weapons;
    bool canAttack() const;
    bool uncompleted = false;
    bool isBuilding = false;
    // This is the build power being USED on this units construction
    float buildPower = 0.0f;

    // This mutext may have to exist in the future
    // std::mutex customStateMutex;
    std::map<std::string, uint32_t> customState;

    uint framesAlive = 0;

    bool operator==(const Instance& other) const;
    bool operator==(uint32_t id) const;
    bool operator<(uint32_t id) const;
    bool operator>(uint32_t id) const;

    void doCrc(boost::crc_32_type& crc) const;
private:
    InstanceUBO _state;
};

struct InstanceComparator {
    inline bool operator()(const Instance *a, const InstanceID& b) const {
          return *a < b;
    }
    inline bool operator()(const InstanceID& a, const Instance *b) const {
          return *b > a;
    }
};