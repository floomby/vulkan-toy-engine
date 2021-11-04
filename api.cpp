#include "engine.hpp"

Engine *Api::context = nullptr;

void Api::cmd_move(const uint32_t unitID, const glm::vec3& destination, const InsertionMode mode) {
    const auto& inst = context->currentScene->getInstance(unitID);
    const auto cmd = Command { CommandKind::MOVE, inst->id, { destination, inst->id } };
    switch (mode) {
        case InsertionMode::BACK:
            inst->commandList.push_back(std::move(cmd));
            break;
        case InsertionMode::FRONT:
            inst->commandList.push_front(std::move(cmd));
            break;
        case InsertionMode::OVERWRITE:
            inst->commandList.clear();
            inst->commandList.push_back(std::move(cmd));
            break;
    }

    // for (auto s : enumNames<CommandKind>()) {
    //     std::cout << s << std::endl;
    // }

    // std::cout << "!" << getEnumString<CommandKind, (CommandKind)0>() << "!" << std::endl;
}

void Api::cmd_stop(const uint32_t unitID, const InsertionMode mode) {
const auto& inst = context->currentScene->getInstance(unitID);
    const auto cmd = Command { CommandKind::STOP, inst->id, { glm::vec3(0.0f), inst->id } };
    switch (mode) {
        case InsertionMode::BACK:
            inst->commandList.push_back(std::move(cmd));
            break;
        case InsertionMode::FRONT:
            inst->commandList.push_front(std::move(cmd));
            break;
        case InsertionMode::OVERWRITE:
            inst->commandList.clear();
            inst->commandList.push_back(std::move(cmd));
            break;
    }
}

#include <iostream>

void Api::eng_echo(const char *message) {
    std::string msg(message);
    msg += '\n';
    std::cout << msg << std::flush;
}