#pragma once

#include "utilities.hpp"

enum class IEngage;

class Entity;
class Instance;

class Base;

// I am using clang and ruby to look at the declarations in this class to generate code which creates bindings for these functions in the global lua namespace
// The other thing is everything here is reentrant
class Api {
public:
    Api() = delete;
    static Base *context;

    static void cmd_move(const InstanceID unitID, const glm::vec3& destination, const InsertionMode mode);
    static void cmd_stop(const InstanceID uintID, const InsertionMode mode);
    static void cmd_setTargetLocation(Instance *instance, const glm::vec3& location);
    // static void cmd_attack(const uint32_t unitID, const uint32_t target, const InsertionMode mode);

    static void cmd_createInstance(const std::string& name, const glm::vec3& position, const glm::quat& heading, TeamID team);
    static void cmd_destroyInstance(InstanceID unitID);

    static void eng_createBallisticProjectile(Entity *projectileEntity, const glm::vec3& position, const glm::vec3& normedDirection, uint32_t parentID);
    static void eng_echo(const char *message);
    static int eng_getTeamID(InstanceID unitID);
    static std::vector<uint32_t> eng_getSelectedInstances();
    static void eng_setInstanceStateEngage(InstanceID unitID, IEngage state);
    static void eng_setInstanceHealth(InstanceID uintID, float health);
    static float eng_getInstanceHealth(InstanceID unitID);
    static std::string eng_getInstanceEntityName(InstanceID unitID);

    static void gui_setVisibility(const char *name, bool visibility);
    static void gui_setLabelText(const std::string& name, const std::string& text);

    static void state_dumpAuthStateIDs();

    static void net_declareTeam(TeamID team, const std::string& name);
};