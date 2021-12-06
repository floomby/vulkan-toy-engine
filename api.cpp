#include "engine.hpp"

Base *Api::context = nullptr;

const std::string invalidStr = "<invalid id>";

void Api::cmd_move(const InstanceID unitID, const glm::vec3& destination, const InsertionMode mode) {
    ApiProtocol data { ApiProtocolKind::COMMAND, 0, "", { CommandKind::MOVE, unitID, { destination, {}, unitID }, mode }};
    context->send(data);
}

void Api::cmd_stop(const InstanceID unitID, const InsertionMode mode) {
    ApiProtocol data { ApiProtocolKind::COMMAND, 0, "", { CommandKind::STOP, unitID, { {}, {}, unitID }, mode }};
    context->send(data);
}

void Api::cmd_createInstance(const std::string& name, const glm::vec3& position, const glm::quat& heading, TeamID team) {
    ApiProtocol data { ApiProtocolKind::COMMAND, 0, "", { CommandKind::CREATE, 0, { position, heading, (uint32_t)team }, InsertionMode::NONE }};
    strncpy(data.buf, name.c_str(), ApiTextBufferSize);
    data.buf[ApiTextBufferSize - 1] = '\0';
    context->send(data);
}

void Api::cmd_destroyInstance(InstanceID unitID) {
    ApiProtocol data { ApiProtocolKind::COMMAND, 0, "", { CommandKind::DESTROY, 0, { {}, {}, 0 }, InsertionMode::NONE }};
    context->send(data);
}

void Api::cmd_setTargetLocation(InstanceID unitID, glm::vec3&& location, InsertionMode mode) {
    ApiProtocol data { ApiProtocolKind::COMMAND, 0, "", { CommandKind::TARGET_LOCATION, 0, { location, {}, 0 }, mode }};
    context->send(data);
}

void Api::cmd_setTargetID(InstanceID unitID, InstanceID targetID, InsertionMode mode) {
    ApiProtocol data { ApiProtocolKind::COMMAND, 0, "", { CommandKind::TARGET_LOCATION, 0, { {}, {}, targetID }, mode }};
    context->send(data);
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

void Api::eng_echo(const char *message) {
    std::string msg("  > ");
    msg += message;
    msg += '\n';
    std::cout << msg << std::flush;
}

std::vector<InstanceID> Api::eng_getSelectedInstances() {
    std::scoped_lock l(context->apiLock);
    return static_cast<Engine *>(context)->idsSelected;
}

int Api::eng_getTeamID(InstanceID unitID) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return -1;
    return it->id;
}

void Api::eng_setInstanceStateEngage(InstanceID unitID, IEngage state) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return;
    it->state.engageKind = state;
}

void Api::eng_setInstanceHealth(InstanceID unitID, float health) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return;
    it->health = health;
}

float Api::eng_getInstanceHealth(InstanceID unitID) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return -1.0f;
    return it->health;
}

float Api::engS_getInstanceHealth(Instance *unit) {
    return unit->health;
}

double Api::eng_getInstanceResources(InstanceID unitID) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return -1.0f;
    return it->resources;
}

double Api::engS_getInstanceResources(Instance *unit) {
    return unit->resources;
}

const std::string& Api::eng_getInstanceEntityName(InstanceID unitID) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return invalidStr;
    return it->entity->name;
}

const std::string& Api::engS_getInstanceEntityName(Instance *unit) {
    return unit->entity->name;
}

InstanceID Api::engS_getInstanceID(Instance *unit) {
    return unit->id;
}

void Api::eng_quit() {
    context->quit();
}

bool Api::eng_getCollidability(InstanceID unitID) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return false;
    return it->hasCollision;
}

bool Api::engS_getCollidability(Instance *unit) {
    return unit->hasCollision;
}

void Api::eng_setCollidability(InstanceID unitID, bool collidability) {
    std::scoped_lock l(context->authState.lock);
    auto it = std::lower_bound(context->authState.instances.begin(), context->authState.instances.end(), unitID);
    if (it == context->authState.instances.end() || *it != unitID) return;
    it->hasCollision = collidability;
}

void Api::engS_setCollidability(Instance *unit, bool collidability) {
    unit->hasCollision = collidability;
}

void Api::gui_setVisibility(const char *name, bool visibility) {
    GuiCommandData *what = new GuiCommandData();
    what->str = name;
    what->action = visibility;
    what->flags = GUIF_NAMED;
    context->gui->submitCommand({ Gui::GUI_VISIBILITY, what });
}

void Api::gui_setLabelText(const std::string& name, const std::string& text) {
    GuiCommandData *what = new GuiCommandData();
    what->str = name;
    what->str2 = text;
    what->flags = GUIF_NAMED;
    context->gui->submitCommand({ Gui::GUI_TEXT, what });
}

void Api::state_dumpAuthStateIDs() {
    std::scoped_lock l(context->authState.lock);
    context->authState.dump();
}

void Api::state_giveResources(TeamID team, double resourceUnits) {
    ApiProtocol data { ApiProtocolKind::RESOURCES, team };
    data.dbl = resourceUnits;
    context->send(data);
}

double Api::state_getResources(TeamID teamID) {
    std::scoped_lock l(context->authState.lock);
    auto it = find(context->authState.teams.begin(), context->authState.teams.end(), teamID);
    if (it != context->authState.teams.end()) return it->resourceUnits;
    return 0.0;
}

void Api::net_declareTeam(TeamID teamID, const std::string& name) {
    ApiProtocol data { ApiProtocolKind::TEAM_DECLARATION, teamID };
    strncpy(data.buf, name.c_str(), ApiTextBufferSize);
    data.buf[ApiTextBufferSize - 1] = '\0';
    context->send(data);
}

void Api::net_pause(bool pause) {
    ApiProtocol data { ApiProtocolKind::PAUSE, (uint64_t)pause };
    context->send(data);
}