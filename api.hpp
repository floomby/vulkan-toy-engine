#pragma once

#include "utilities.hpp"

class Engine;

enum class InsertionMode {
    FRONT,
    BACK,
    OVERWRITE,
    COUNT
};

class Api {
public:
    Api() = delete;
    static Engine *context;
    static void cmd_move(const uint32_t unitID, const glm::vec3& destination, const InsertionMode mode);
    // static void cmd_attack(const uint32_t unitID, const uint32_t target, const InsertionMode mode);
    static void cmd_stop(const uint32_t uintID, const InsertionMode mode);
    static void eng_echo(const char *message);
};