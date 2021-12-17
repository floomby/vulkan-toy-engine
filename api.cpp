#include "engine.hpp"
#include "sound.hpp"

Base *Api::context = nullptr;

#include "api_util.hpp"

unsigned long ApiUtil::getCallbackID() {
    return ++ApiUtil::callbackCounter;
}

thread_local std::queue<CallbackID> ApiUtil::callbackIds;

static std::mutex dispatchLock;
static std::mutex threadDispatchLock;

void ApiUtil::doCallbackDispatch(CallbackID id, const ApiProtocol& data) {
    dispatchLock.lock();
    auto it = ApiUtil::callbacks.find(id);
    if (it == ApiUtil::callbacks.end()) {
        std::cerr << "Missing callback (this probably indicates engine programmer error)" << std::endl;
        dispatchLock.unlock();
        return;
    }
    auto tmp = it->second;
    ApiUtil::callbacks.erase(it);
    dispatchLock.unlock();
    if (tmp.second == LuaWrapper::threadLuaInstance) {
        tmp.first(data);
        return;
    }
    std::scoped_lock l(threadDispatchLock);
    ApiUtil::otherThreadsData.insert({ tmp.second, { tmp.first, data }});
}

void LuaWrapper::dispatchCallbacks() {
    std::vector<std::pair<std::function<void (const ApiProtocol&)>, const ApiProtocol>> todo;
    assert(LuaWrapper::threadLuaInstance);
    threadDispatchLock.lock();
    // I think the stl has something to make this a one liner and more efficiently, but I am too dumb rn to figure it out
    auto it = ApiUtil::otherThreadsData.find(LuaWrapper::threadLuaInstance);
    while (it != ApiUtil::otherThreadsData.end()) {
        if (Api::context->authState.frame > it->second.second.frame) {
            todo.push_back(it->second);
            ApiUtil::otherThreadsData.erase(it++);
        } else it++;
    }
    threadDispatchLock.unlock();
    for (const auto [func, data] : todo) {
        func(data);
    }
}

#define lock_and_get_iterator std::scoped_lock l(context->authState.lock); \
    auto itx = context->authState.getInstance(unitID); \
    if (itx == context->authState.instances.end() || (*itx)->id != unitID) throw std::runtime_error("Invalid instance id"); \
    auto it = *itx;

void Api::cmd_move(const InstanceID unitID, const glm::vec3& destination, const InsertionMode mode) {
    ApiProtocol data { ApiProtocolKind::COMMAND, 0, "", { CommandKind::MOVE, unitID, { destination, {}, unitID }, mode }};
    context->send(data);
}

void Api::cmd_stop(const InstanceID unitID, const InsertionMode mode) {
    ApiProtocol data { ApiProtocolKind::COMMAND, 0, "", { CommandKind::STOP, unitID, { {}, {}, unitID }, mode }};
    context->send(data);
}

void Api::cmd_createInstance(const std::string& name, const glm::vec3& position, const glm::quat& heading, TeamID team, std::function<void (InstanceID)> cb) {
    // I will fix this to work with c++ callbacks, don't worry, but this is a check to make sure that we are just getting lua callbacks like we expect
    assert(ApiUtil::callbackIds.size() == 1);
    auto cbID = ApiUtil::callbackIds.front();
    ApiUtil::callbackIds.pop();

    ApiProtocol data { ApiProtocolKind::COMMAND, 0, "", { CommandKind::CREATE, 0, { position, heading, (uint32_t)team }, InsertionMode::NONE }};
    strncpy(data.buf, name.c_str(), ApiTextBufferSize);
    data.buf[ApiTextBufferSize - 1] = '\0';
    data.callbackID = cbID;
    context->send(data);

    if (cbID) {
        assert(LuaWrapper::threadLuaInstance);
        std::scoped_lock l(dispatchLock);
        ApiUtil::callbacks.insert({ cbID, { [cb](const ApiProtocol& data) { cb(data.command.id); }, LuaWrapper::threadLuaInstance }});
    }
}

void Api::cmd_destroyInstance(InstanceID unitID) {
    ApiProtocol data { ApiProtocolKind::COMMAND, 0, "", { CommandKind::DESTROY, unitID, { {}, {}, 0 }, InsertionMode::NONE }};
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

void Api::cmd_build(InstanceID unitID, const char *what, InsertionMode mode) {
    if (!Api::context->currentScene->entities.contains(what)) throw std::runtime_error("Invalid build order");
    ApiProtocol data { ApiProtocolKind::COMMAND, 0, "", { CommandKind::BUILD, unitID, { {}, {}, 0 }, mode }};
    strncpy(data.command.data.buf, what, CommandDataBufSize);
    data.command.data.buf[CommandDataBufSize - 1] = '\0';
    context->send(data);
}

void Api::eng_createBallisticProjectile(Entity *projectileEntity, const glm::vec3& position, const glm::vec3& normedDirection, uint32_t parentID) {
    std::scoped_lock l(context->authState.lock);
    Instance *inst;
    if (context->headless) {
        inst = new Instance(projectileEntity, context->authState.counter++);
    } else {
        inst = new Instance(projectileEntity, context->currentScene->textures.data() + projectileEntity->textureIndex,
            context->currentScene->models.data() + projectileEntity->modelIndex, context->authState.counter++, true);
    }
    inst->dP = normedDirection * projectileEntity->maxSpeed;
    inst->position = position;
    inst->heading = { 1.0f, 0.0f, 0.0f, 0.0f };
    inst->parentID = parentID;
    context->authState.instances.push_back(inst);
}

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
    lock_and_get_iterator
    return it->id;
}

void Api::eng_setInstanceStateEngage(InstanceID unitID, IEngage state) {
    lock_and_get_iterator
    it->state.engageKind = state;
}

void Api::eng_setInstanceHealth(InstanceID unitID, float health) {
    lock_and_get_iterator
    it->health = health;
}

float Api::eng_getInstanceHealth(InstanceID unitID) {
    lock_and_get_iterator
    return it->health;
}

float Api::engS_getInstanceHealth(Instance *unit) {
    return unit->health;
}

double Api::eng_getInstanceResources(InstanceID unitID) {
    lock_and_get_iterator
    return it->resources;
}

double Api::engS_getInstanceResources(Instance *unit) {
    return unit->resources;
}

const std::string& Api::eng_getInstanceEntityName(InstanceID unitID) {
    lock_and_get_iterator
    return it->entity->name;
}

const std::string& Api::engS_getInstanceEntityName(Instance *unit) {
    return unit->entity->name;
}

InstanceID Api::engS_getInstanceID(Instance *unit) {
    return unit->id;
}

float Api::engS_getRandomF() {
    return context->authState.realDistribution(context->authState.randomGenerator);
}

void Api::eng_quit() {
    context->quit();
}

bool Api::eng_getCollidability(InstanceID unitID) {
    lock_and_get_iterator
    return it->hasCollision;
}

bool Api::engS_getCollidability(Instance *unit) {
    return unit->hasCollision;
}

void Api::eng_setCollidability(InstanceID unitID, bool collidability) {
    lock_and_get_iterator
    it->hasCollision = collidability;
}

void Api::engS_setCollidability(Instance *unit, bool collidability) {
    unit->hasCollision = collidability;
}

bool Api::eng_instanceCanBuild(InstanceID unitID) {
    lock_and_get_iterator
    return !it->entity->buildOptions.empty();
}

bool Api::engS_instanceCanBuild(Instance *unit) {
    return !unit->entity->buildOptions.empty();
}

std::vector<std::string>& Api::eng_getInstanceBuildOptions(InstanceID unitID) {
    lock_and_get_iterator
    return it->entity->buildOptions;
}

std::vector<std::string>& Api::engS_getInstanceBuildOptions(Instance *unit) {
    return unit->entity->buildOptions;
}

std::vector<std::string>& Api::eng_getEntityBuildOptions(Entity *entity) {
    return entity->buildOptions;
}

Entity *Api::eng_getInstanceEntity(InstanceID unitID) {
    lock_and_get_iterator
    return it->entity;
}

Entity *Api::engS_getInstanceEntity(Instance *unit) {
    return unit->entity;
}

Entity *Api::eng_getEntityFromName(const char *name) {
    const auto ent = context->currentScene->entities.find(name);
    if (ent == context->currentScene->entities.end()) {
        std::string msg = "Unable to find entity ";
        msg += name;
        throw std::runtime_error(msg);
    }
    return ent->second;
}

// TODO The sound api stuff needs to be made threadsafe
std::vector<std::string> Api::eng_listAudioDevices() {
    if (!context->sound) throw std::runtime_error("Sound is not enabled in this context");
    return context->sound->listDevices();
}

void Api::eng_pickAudioDevice(const char *name) {
    if (!context->sound) throw std::runtime_error("Sound is not enabled in this context");
    context->sound->setDevice(name);
}

void Api::eng_playSound(const char *name) {
    if (!context->sound) throw std::runtime_error("Sound is not enabled in this context");
    context->sound->playSound(name);
}

void Api::eng_setInstanceCustomState(InstanceID unitID, std::string key, int value) {
    lock_and_get_iterator
    it->customState[key] = value;
}

void Api::engS_setInstanceCustomState(Instance *unit, std::string key, int value) {
    std::scoped_lock l(context->authState.lock);
    unit->customState[key] = value;
}

std::optional<int> Api::eng_getInstanceCustomState(InstanceID unitID, std::string key) {
    lock_and_get_iterator
    auto mit = it->customState.find(key);
    if (mit == it->customState.end()) return {};
    return std::optional<int>(mit->second);
}

std::optional<int> Api::engS_getInstanceCustomState(Instance *unit, std::string key) {
    std::scoped_lock l(context->authState.lock);
    auto mit = unit->customState.find(key);
    if (mit == unit->customState.end()) return {};
    return std::optional<int>(mit->second);
}

void Api::eng_removeInstanceCustomState(InstanceID unitID, std::string key) {
    lock_and_get_iterator
    auto mit = it->customState.find(key);
    if (mit == it->customState.end()) return;
    it->customState.erase(mit);
}

void Api::engS_removeInstanceCustomState(Instance *unit, std::string key) {
    std::scoped_lock l(context->authState.lock);
    auto mit = unit->customState.find(key);
    if (mit == unit->customState.end()) return;
    unit->customState.erase(mit);
}

glm::vec3 Api::eng_getInstancePosition(InstanceID unitID) {
    lock_and_get_iterator
    return it->position;
}

glm::vec3& Api::engS_getInstancePosition(Instance *unit) {
    return unit->position;
}

glm::quat Api::eng_getInstanceHeading(InstanceID unitID) {
    lock_and_get_iterator
    return it->heading;
}

glm::quat& Api::engS_getInstanceHeading(Instance *unit) {
    return unit->heading;
}

unsigned long Api::eng_frame() {
    return context->authState.frame;
}

void Api::gui_setVisibility(const char *name, bool visibility) {
    assert(!context->headless);
    GuiCommandData *what = new GuiCommandData();
    what->str = name;
    what->action = visibility;
    what->flags = GUIF_NAMED;
    context->gui->submitCommand({ Gui::GUI_VISIBILITY, what });
}

void Api::gui_setLabelText(const std::string& name, const std::string& text) {
    assert(!context->headless);
    GuiCommandData *what = new GuiCommandData();
    what->str = name;
    what->str2 = text;
    what->flags = GUIF_NAMED;
    context->gui->submitCommand({ Gui::GUI_TEXT, what });
}

void Api::gui_addPanel(const char *root, const char *tableName) {
    assert(!context->headless);
    GuiCommandData *what = new GuiCommandData();
    what->str = root;
    what->str2 = tableName;
    what->flags = GUIF_NAMED | GUIF_LUA_TABLE;
    context->gui->submitCommand({ Gui::GUI_LOAD, what });
    GuiCommandData *what2 = new GuiCommandData();
    what2->str = tableName;
    what2->action = GUIA_VISIBLE;
    what2->flags = GUIF_PANEL_NAME;
    context->gui->submitCommand({ Gui::GUI_VISIBILITY, what2 });
}

void Api::gui_removePanel(const char *panelName) {
    assert(!context->headless);
    GuiCommandData *what = new GuiCommandData();
    what->str = panelName;
    what->flags = GUIF_PANEL_NAME;
    // This is a reminder that I will probabl want to implement this in the future
    what->childIndices = {};
    context->gui->submitCommand({ Gui::GUI_REMOVE, what });
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
    if (teamID > Config::maxTeams) {
        std::string msg = "TeamID is out of bounds (";
        msg += teamID;
        msg += " is greater than max teams)";
        throw std::runtime_error(msg);
    }

    auto tmp = context->authState.teams[teamID];
    if (!tmp) {
        std::string msg = "TeamID is invalid (";
        msg += teamID;
        msg += " has not been declared)";
        throw std::runtime_error(msg);
    }
    return tmp->resourceUnits;
}

std::pair<TeamID, const std::string> Api::state_getTeamIAm() {
    assert(!context->headless);
    return { static_cast<Engine *>(context)->engineSettings.teamIAm.id,
        static_cast<Engine *>(context)->engineSettings.teamIAm.displayName };
}

void Api::net_declareTeam(TeamID teamID, const std::string& name) {
    if (teamID > Config::maxTeams) {
        std::string msg = "TeamID is out of bounds (";
        msg += teamID;
        msg += " is greater than max teams)";
        throw std::runtime_error(msg);
    }
    ApiProtocol data { ApiProtocolKind::TEAM_DECLARATION, teamID };
    strncpy(data.buf, name.c_str(), ApiTextBufferSize);
    data.buf[ApiTextBufferSize - 1] = '\0';
    context->send(data);
}

void Api::net_pause(bool pause) {
    ApiProtocol data { ApiProtocolKind::PAUSE, (uint64_t)pause };
    context->send(data);
}