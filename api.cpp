#include "engine.hpp"

Engine *Api::context = nullptr;

void Api::cmd_move(const uint32_t unitID, const glm::vec3& destination, const InsertionMode mode) {
    // std::scoped_lock(context->currentScene->state.apiLock);
    const auto& inst = context->authState.getInstance(unitID);
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
}

void Api::cmd_stop(const uint32_t unitID, const InsertionMode mode) {
    // std::scoped_lock(context->currentScene->state.apiLock);
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

void Api::eng_createInstance(const std::string& name, const glm::vec3& position, const glm::quat& heading) {
    // Instance(entities.data() + entityIndex, textures.data() + entityIndex, models.data() + entityIndex, entityIndex)
    auto ent = context->currentScene->entities[name];
    Instance inst(ent, context->currentScene->textures.data() + ent->textureIndex, context->currentScene->models.data() + ent->modelIndex, true);
    // lock it here
    inst.id = context->authState.counter++; // I dont like this line, it creates races, although technically the others race as well
    // I probably just need a way to ensure order of api calls is consistent across every copy of the game like uuid sorting or something
    inst.heading = heading;
    inst.position = position;
    context->authState.instances.push_back(std::move(inst));
}

#include <iostream>

void Api::eng_echo(const char *message) {
    std::string msg(">>");
    msg += message;
    msg += '\n';
    std::cout << msg << std::flush;
}