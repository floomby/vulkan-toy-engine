#pragma once

#include "utilities.hpp"

enum class IEngage;

class Entity;
class Instance;

class Base;

// I am using clang and ruby to look at the declarations in this class to generate code which creates bindings for these functions in the global lua namespace
// Importantly everything here needs to be threadsafe except the *S_* functions which are allow to be used in unitais run in the main update loop (which is not parallelized)
class Api {
public:
    Api() = delete;
    static Base *context;

    static void cmd_move(const InstanceID unitID, const glm::vec3& destination, const InsertionMode mode);
    static void cmd_setTargetLocation(InstanceID unitID, glm::vec3&& location, InsertionMode mode);
    static void cmd_setTargetID(InstanceID unitID, InstanceID targetID, InsertionMode mode);
    static void cmd_createInstance(const std::string& name, const glm::vec3& position, const glm::quat& heading, TeamID team, std::function<void(InstanceID)> cb);
    static void cmd_stop(const InstanceID uintID, const InsertionMode mode);
    static void cmd_destroyInstance(InstanceID unitID);
    static void cmd_build(InstanceID unitID, const char *what, InsertionMode mode);
    static void cmd_buildStation(InstanceID unitID, const glm::vec3& where, InsertionMode mode, const char *what);
    // static void cmd_attack(InstanceID unitID, InstanceID target, InsertionMode mode);

    // I dont like how I "namespaced" these function, it seems like almost everything is an eng* function
    static void eng_createBallisticProjectile(Entity *projectileEntity, const glm::vec3& position, const glm::vec3& normedDirection, InstanceID parentID);
    static void eng_createGuidedProjectile(Entity *projectileEntity, const glm::vec3& position, const glm::vec3& normedDirection, InstanceID parentID, TeamID teamID);
    static void eng_createBeam(uint32_t color, float damage, const glm::vec3& from, const glm::vec3& to, InstanceID parentID);
    static void eng_echo(const char *message);
    static int eng_getTeamID(InstanceID unitID);
    static std::vector<InstanceID> eng_getSelectedInstances();
    static void eng_setInstanceStateEngage(InstanceID unitID, IEngage state);
    static void eng_setInstanceHealth(InstanceID uintID, float health);
    static float eng_getInstanceHealth(InstanceID unitID);
    static float engS_getInstanceHealth(Instance *unit);
    static double eng_getInstanceResources(InstanceID unitID);
    static double engS_getInstanceResources(Instance *unit);
    static const std::string& eng_getInstanceEntityName(InstanceID unitID);
    static const std::string& engS_getInstanceEntityName(Instance *unit);
    static InstanceID engS_getInstanceID(Instance *unit);
    static bool eng_getCollidability(InstanceID unitID);
    static bool engS_getCollidability(Instance *unit);
    static void eng_setCollidability(InstanceID unitID, bool collidability);
    static void engS_setCollidability(Instance *unit, bool collidability);
    static bool eng_instanceCanBuild(InstanceID unitID);
    static bool engS_instanceCanBuild(Instance *unit);
    static std::vector<std::string>& eng_getInstanceBuildOptions(InstanceID unitID);
    static std::vector<std::string>& engS_getInstanceBuildOptions(Instance *unit);
    static std::vector<std::string>& eng_getEntityBuildOptions(Entity *entity);
    static Entity *eng_getInstanceEntity(InstanceID unitID);
    static Entity *engS_getInstanceEntity(Instance *unit);
    static Entity *eng_getEntityFromName(const char *name);
    static std::vector<std::string> eng_listAudioDevices();
    static void eng_pickAudioDevice(const char *name);
    static void eng_playSound(const char *name);
    static void eng_playSound3d(const char *name, const glm::vec3& position, const glm::vec3& velocity);
    static void eng_setInstanceCustomState(InstanceID unitID, std::string key, int value);
    static void engS_setInstanceCustomState(Instance *unit, std::string key, int value);
    static std::optional<int> eng_getInstanceCustomState(InstanceID unitID, std::string key);
    static std::optional<int> engS_getInstanceCustomState(Instance *unit, std::string key);
    static void eng_removeInstanceCustomState(InstanceID unitID, std::string key);
    static void engS_removeInstanceCustomState(Instance *unit, std::string key);
    static glm::vec3 eng_getInstancePosition(InstanceID unitID);
    static glm::vec3& engS_getInstancePosition(Instance *unit);
    static glm::quat eng_getInstanceHeading(InstanceID unitID);
    static glm::quat& engS_getInstanceHeading(Instance *unit);
    static unsigned long eng_frame();
    static float engS_getRandomF();
    static void eng_quit();
    static float eng_fps();
    static void eng_declareKeyBinding(int key);
    static void eng_undeclareKeyBinding(int key);
    // These two aren't really thread safe, but the consequences of that are minimal
    static int eng_getScreenWidth();
    static int eng_getScreenHeight();
    // read setCursorEntity as "we are placing a structure to build now" (I don't like how I did this)
    static void eng_setCursorEntity(const std::string& name);
    static void eng_clearCursorEntity();
    static bool eng_entityIsStation(const std::string& name);

    static void gui_setVisibility(const char *name, bool visibility);
    static void gui_setLabelText(const std::string& name, const std::string& text);
    // Idk what I am doing, this seems weird and arbitrarly limiting
    static void gui_addPanel(const char *root, const char *tableName);
    static void gui_removePanel(const char *panelName);

    static void state_dumpAuthStateIDs();
    static void state_giveResources(TeamID teamID, double resourceUnits);
    static double state_getResources(TeamID teamID);
    static std::pair<TeamID, const std::string> state_getTeamIAm();

    static void net_declareTeam(TeamID teamID, const std::string& name);
    static void net_declareNullTeam(TeamID teamID, const std::string& name);
    static void net_pause(bool pause);

    static glm::vec4 util_colorIntToVec(uint32_t color);
};