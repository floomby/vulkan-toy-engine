#pragma once

#include "utilities.hpp"

class Entity;
class Instance;

class Engine;

enum class InsertionMode {
    FRONT,
    BACK,
    OVERWRITE,
    COUNT
};

// I am using clang and ruby to look at the declarations in this class to generate code which creates bindings for these functions in the global lua namespace 
class Api {
public:
    Api() = delete;
    static Engine *context;
    static void cmd_move(const uint32_t unitID, const glm::vec3& destination, const InsertionMode mode);
    // static void cmd_attack(const uint32_t unitID, const uint32_t target, const InsertionMode mode);
    static void cmd_stop(const uint32_t uintID, const InsertionMode mode);
    static void eng_createInstance(const std::string& name, const glm::vec3& position, const glm::quat& heading, int team);
    static void eng_createBallisticProjectile(Entity *projectileEntity, const glm::vec3& position, const glm::vec3& normedDirection);
    static void eng_echo(const char *message);
    static void test_fire();
    static void cmd_setTargetLocation(Instance *instance, const glm::vec3& location);
    // I see no way to handle the pointers being invalidated without much jankyness (using special values on the heap and checking if they are 
    // there after polling the os to see if that portion if the heap is still mapped in virtual memory)
    // static std::vector<Instance *> eng_getSelectedInstances();
    static std::vector<uint32_t> eng_getSelectedInstances();
    static int eng_getTeamID(uint32_t unitID);
};