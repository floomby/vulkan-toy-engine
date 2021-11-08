#pragma once

#include "utilities.hpp"

class Engine;

enum class InsertionMode {
    FRONT,
    BACK,
    OVERWRITE,
    COUNT
};

// For right now I am making the lua bindings by hand, but I will probably make a code generator in some scripting language for making the bindings in the future
// I can use the __PRETTY_FUNCTION__ compiler feature to get the types easily
class Api {
public:
    Api() = delete;
    static Engine *context;
    static void cmd_move(const uint32_t unitID, const glm::vec3& destination, const InsertionMode mode);
    // static void cmd_attack(const uint32_t unitID, const uint32_t target, const InsertionMode mode);
    static void cmd_stop(const uint32_t uintID, const InsertionMode mode);
    static void eng_createInstance(const std::string& name, const glm::vec3& position, const glm::vec3& heading);
    static void eng_echo(const char *message);
};