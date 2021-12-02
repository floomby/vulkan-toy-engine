#include "engine.hpp"

Base *Api::context = nullptr;

void Api::cmd_move(const uint32_t unitID, const glm::vec3& destination, const InsertionMode mode) {
    std::scoped_lock l(context->authState.lock);
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
    std::scoped_lock l(context->authState.lock);
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

uint32_t Api::eng_createInstance(const std::string& name, const glm::vec3& position, const glm::quat& heading, int team) {
    std::scoped_lock l(context->authState.lock);
    // Instance(entities.data() + entityIndex, textures.data() + entityIndex, models.data() + entityIndex, entityIndex)
    const auto ent = context->currentScene->entities.at(name);
    Instance inst(ent, context->currentScene->textures.data() + ent->textureIndex, context->currentScene->models.data() + ent->modelIndex,
        context->authState.counter++, true);
    // lock it here
    // I probably just need a way to ensure order of api calls is consistent across every copy of the game like uuid sorting or something
    inst.heading = heading;
    inst.position = position;
    inst.team = team;
    context->authState.instances.push_back(std::move(inst));
    return inst.id;
}

void Api::eng_createBallisticProjectile(Entity *projectileEntity, const glm::vec3& position, const glm::vec3& normedDirection, uint32_t parentID) {
    // std::scoped_lock l(context->authState.lock);
    std::scoped_lock l(context->authState.lock);
    Instance inst(projectileEntity, context->currentScene->textures.data() + projectileEntity->textureIndex,
        context->currentScene->models.data() + projectileEntity->modelIndex, context->authState.counter++, true);
    inst.dP = normedDirection * projectileEntity->maxSpeed;
    inst.position = position;
    inst.heading = { 1.0f, 0.0f, 0.0f, 0.0f };
    inst.parentID = parentID;
    // std::cout << inst.entity << std::endl;
    context->authState.instances.push_back(std::move(inst));
    // context->authState.dump();
}

#include <iostream>

void Api::cmd_setTargetLocation(Instance *instance, const glm::vec3& location) {
    std::scoped_lock l(context->authState.lock);
    instance->target = Target(location);
}

void Api::eng_echo(const char *message) {
    std::string msg("  > ");
    msg += message;
    msg += '\n';
    std::cout << msg << std::flush;
}

// void Api::test_fire() {
//     std::scoped_lock l(context->authState.lock);
//     auto ent = context->currentScene->entities["basic_plasma"];
//     Instance inst(ent, context->currentScene->textures.data() + ent->textureIndex, context->currentScene->models.data() + ent->modelIndex, true);
//     inst.dP = { ent->maxSpeed, 0.0f, 0.0f };
//     inst.position = { 0.0f, 0.0f, 0.0f };
//     inst.heading = { 1.0f, 0.0f, 0.0f, 0.0f };
//     inst.id = std::numeric_limits<uint32_t>::max();
//     context->authState.instances.push_back(std::move(inst));
// }

std::vector<uint32_t> Api::eng_getSelectedInstances() {
    std::scoped_lock l(context->apiLock);
    return static_cast<Engine *>(context)->idsSelected;
}

int Api::eng_getTeamID(uint32_t unitID) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return -1;
    return it->id;
}

void Api::gui_setVisibility(const char *name, bool visibility) {
    GuiCommandData *what = new GuiCommandData();
    what->str = name;
    what->action = visibility;
    what->flags = GUIF_NAMED;
    context->gui->submitCommand({ Gui::GUI_VISIBILITY, what });
}

void Api::eng_setInstanceStateEngage(uint32_t unitID, IEngage state) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return;
    it->state.engageKind = state;
}

void Api::eng_setInstanceHealth(uint32_t unitID, float health) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return;
    it->health = health;
}

float Api::eng_getInstanceHealth(uint32_t unitID) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return -1.0f;
    return it->health;
}

void Api::state_dumpAuthStateIDs() {
    std::scoped_lock l(context->authState.lock);
    context->authState.dump();
}

std::string Api::eng_getInstanceEntityName(uint32_t unitID) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return "<invalid id>";
    return it->entity->name;
}

void Api::gui_setLabelText(const std::string& name, const std::string& text) {
    GuiCommandData *what = new GuiCommandData();
    what->str = name;
    what->str2 = text;
    what->flags = GUIF_NAMED;
    context->gui->submitCommand({ Gui::GUI_TEXT, what });
}

void Api::net_declareTeam(int team, const std::string& name) {
    ApiProtocol data = { ApiProtocolKind::TEAM_DECLARATION };
    strncpy(data.buf, name.c_str(), ApiTextBufferSize);
    data.buf[ApiTextBufferSize - 1] = '\0';
    context->send(data);
}
