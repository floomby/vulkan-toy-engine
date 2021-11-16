#include "engine.hpp"

Engine *Api::context = nullptr;

void Api::cmd_move(const uint32_t unitID, const glm::vec3& destination, const InsertionMode mode) {
    std::scoped_lock(context->authState.lock);
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
    std::scoped_lock(context->authState.lock);
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

void Api::eng_createInstance(const std::string& name, const glm::vec3& position, const glm::quat& heading, int team) {
    std::scoped_lock(context->authState.lock);
    // Instance(entities.data() + entityIndex, textures.data() + entityIndex, models.data() + entityIndex, entityIndex)
    auto ent = context->currentScene->entities[name];
    Instance inst(ent, context->currentScene->textures.data() + ent->textureIndex, context->currentScene->models.data() + ent->modelIndex, true);
    // lock it here
    inst.id = context->authState.counter++; // I dont like this line, it creates races, although technically the others race as well
    // I probably just need a way to ensure order of api calls is consistent across every copy of the game like uuid sorting or something
    inst.heading = heading;
    inst.position = position;
    inst.team = team;
    context->authState.instances.push_back(std::move(inst));
}

// 
void Api::eng_createBallisticProjectile(Entity *projectileEntity, const glm::vec3& position, const glm::vec3& normedDirection) {
    std::scoped_lock(context->authState.lock);
    Instance inst(projectileEntity, context->currentScene->textures.data() + projectileEntity->textureIndex,
        context->currentScene->models.data() + projectileEntity->modelIndex, true);
    inst.dP = normedDirection * projectileEntity->maxSpeed;
    inst.position = position;
    inst.heading = { 1.0f, 0.0f, 0.0f, 0.0f };
    inst.id = std::numeric_limits<uint32_t>::max();
    context->authState.instances.push_back(std::move(inst));
}

#include <iostream>

void Api::cmd_setTargetLocation(Instance *instance, const glm::vec3& location) {
    std::scoped_lock(context->authState.lock);
    instance->target = Target(location);
}

void Api::eng_echo(const char *message) {
    std::string msg(">>");
    msg += message;
    msg += '\n';
    std::cout << msg << std::flush;
}

void Api::test_fire() {
    std::scoped_lock(context->authState.lock);
    auto ent = context->currentScene->entities["basic_plasma"];
    Instance inst(ent, context->currentScene->textures.data() + ent->textureIndex, context->currentScene->models.data() + ent->modelIndex, true);
    inst.dP = { ent->maxSpeed, 0.0f, 0.0f };
    inst.position = { 0.0f, 0.0f, 0.0f };
    inst.heading = { 1.0f, 0.0f, 0.0f, 0.0f };
    inst.id = std::numeric_limits<uint32_t>::max();
    context->authState.instances.push_back(std::move(inst));
}