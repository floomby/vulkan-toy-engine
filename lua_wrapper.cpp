#include "lua_wrapper.hpp"
#include "api_util.hpp"
#include "gui.hpp"

thread_local LuaWrapper *LuaWrapper::threadLuaInstance = nullptr;
thread_local bool LuaWrapper::isGuiThread = false;

static int wrap_exceptions(lua_State *ls, lua_CFunction f)
{
    try {
        return f(ls);
    // } catch (const char *s) {
    //     lua_pushstring(ls, s);
    } catch (const std::exception& e) {
        lua_pushstring(ls, e.what());
    }
    return lua_error(ls);
}

LuaWrapper::LuaWrapper(bool rethrowExceptions) {
    luaState = luaL_newstate();
    luaL_openlibs(luaState);
    if (rethrowExceptions) {
        lua_pushlightuserdata(luaState, (void *)wrap_exceptions);
        luaJIT_setmode(luaState, -1, LUAJIT_MODE_WRAPCFUNC | LUAJIT_MODE_ON);
        lua_pop(luaState, 1);
    }
}

LuaWrapper::~LuaWrapper() {
    lua_close(luaState);
}

#include <cstdio>

void LuaWrapper::error(const char *fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    char buf[400];
    vsnprintf(buf, sizeof(buf) - 1, fmt, argp);
    va_end(argp);
    std::string msg = "Lua error: ";
    msg += buf;
    throw std::runtime_error(msg);
}

double LuaWrapper::getNumberField(const char *key) {
    double result;
    lua_pushstring(luaState, key);
    lua_gettable(luaState, -2);
    if (!lua_isnumber(luaState, -1))
        error("Invalid number for field %s", key);
    result = lua_tonumber(luaState, -1);
    lua_pop(luaState, 1);
    return result;
}

// put it on the top of the stack
bool LuaWrapper::getFunctionField(const char *key, int pos) {
    lua_pushstring(luaState, key);
    lua_gettable(luaState, -2 - pos);
    if (!lua_isfunction(luaState, -1)) {
        lua_pop(luaState, 1);
        return false;
    }
    return true;
}

std::string LuaWrapper::getStringField(const char *key) {
    lua_pushstring(luaState, key);
    lua_gettable(luaState, -2);
    if (!lua_isstring(luaState, -1))
        error("Invalid string for field %s", key);
    const char *s = lua_tostring(luaState, -1);
    size_t len = lua_strlen(luaState, -1); // I guess we ignore the length for now
    lua_pop(luaState, 1);
    return std::string(s);
}

void LuaWrapper::getStringField(const char *key, std::string& str) {
    lua_pushstring(luaState, key);
    lua_gettable(luaState, -2);
    if (!lua_isstring(luaState, -1)) {
        lua_pop(luaState, 1);
        return;
    }
    const char *s = lua_tostring(luaState, -1);
    size_t len = lua_strlen(luaState, -1); // I guess we ignore the length for now
    lua_pop(luaState, 1);
    str = s;
}

void LuaWrapper::getStringsField(const char *key, std::vector<std::string>& strs) {
    lua_pushstring(luaState, key);
    lua_gettable(luaState, -2);
    if (!lua_istable(luaState, -1)) {
        lua_pop(luaState, 1);
        return;
    }
    auto count = lua_objlen(luaState, -1);
    strs.reserve(strs.size() + count);
    for (int i = 1; i <= count; i++) {
        lua_rawgeti(luaState, -1, i);
        strs.push_back(luaL_checkstring(luaState, -1));
        lua_pop(luaState, 1);
    }
    lua_pop(luaState, 1);
}

bool LuaWrapper::getBooleanField(const char *key) {
    lua_pushstring(luaState, key);
    lua_gettable(luaState, -2);
    if (!lua_isboolean(luaState, -1)) {
        lua_pop(luaState, 1);
        return false;
    }
    bool ret = lua_toboolean(luaState, -1);
    lua_pop(luaState, 1);
    return ret;
}

GuiLayoutNode *LuaWrapper::loadGuiFile(const char *name) {
    std::string filename = std::string(name);
    std::transform(filename.begin(), filename.end(), filename.begin(), [](unsigned char c) { return std::tolower(c); });
    filename += ".lua";
    if (luaL_loadfile(luaState, filename.c_str()) || lua_pcall(luaState, 0, 0, 0))
        error("Could not load gui file: %s", lua_tostring(luaState, -1));

    return loadGuiTable(name);
}

GuiLayoutNode *LuaWrapper::loadGuiTable(const char *name) {
    lua_getglobal(luaState, name);
    if (!lua_istable(luaState, -1))
        error("%s should be a table", name);

    auto it = std::find_if(luaPanels.begin(), luaPanels.end(), [name](const auto& x){ return x.first == name; });
    if (it != luaPanels.end()) {
        std::cerr << "This panel is already loaded: this likely indicates a lua logic error" << std::endl;
        return nullptr;
    }

    oldHandlers = totalHandlers;
    auto ret = readGuiLayoutNode(0);
    luaPanels.push_back({ name, totalHandlers - oldHandlers });

    return ret;
}

void LuaWrapper::removeGuiTable(const std::string& name) {
    auto offset = getPanelHandlerOffset(name);
    auto it = std::find_if(luaPanels.begin(), luaPanels.end(), [name](const auto& x){ return x.first == name; });
    if (it == luaPanels.end()) {
        std::cerr << "Unable to find panel: " << name << std::endl;
        return;
    }
    totalHandlers -= it->second;
    for (int i = 0; i < it->second; i++) {
        lua_remove(luaState, offset + 1);
    }
    luaPanels.erase(it);
}

uint LuaWrapper::getPanelHandlerOffset(const std::string& name) {
    if (name.empty()) return 0;
    uint acm = 0;
    for (auto it = luaPanels.begin();;it++) {
        if (it == luaPanels.end()) throw std::runtime_error("Unable to find panel: " + name);
        if (it->first == name) return acm;
        acm += it->second;
    }
}

// I am sorry this got so goofy with offset calculation (I could rewrite it to be simpler, but that takes thinking)
#define add_handler(name) if (getFunctionField(name, pushedHandlerCount)) { \
    ret->handlers.insert({ name, lua_gettop(luaState) - oldHandlers - 1 - handlerOffset}); \
    pushedHandlerCount++; totalHandlers++; }

GuiLayoutNode *LuaWrapper::readGuiLayoutNode(int handlerOffset) {
    auto ret = new GuiLayoutNode;
    int tableIndex = lua_gettop(luaState);
    ret->x = getNumberField("x");
    ret->y = getNumberField("y");
    ret->height = getNumberField("height");
    getNumberField("width", ret->width);
    ret->kind = (GuiLayoutKind)(int)getNumberField("kind");
    ret->color = (uint32_t)getNumberField("color");
    getNumberField("secondaryColor", ret->secondaryColor);

    getStringField("text", ret->text);
    getStringField("name", ret->name);
    getStringField("tooltip", ret->tooltip);
    getStringsField("images", ret->imageStates);
    
    int pushedHandlerCount = 0;
    add_handler("onClick")
    add_handler("onToggle")
    add_handler("onSelectionChanged")
    add_handler("onTextUpdated")
    add_handler("onPeriodicUpdate")
    
    lua_pushstring(luaState, "children");
    int toRemove = lua_gettop(luaState);
    lua_gettable(luaState, -2 - pushedHandlerCount);
    if (!lua_isnil(luaState, -1)) {
        if (!lua_istable(luaState, -1))
            error("Children should be a table");
        int childrenCount = lua_objlen(luaState, -1);

        ret->children.reserve(childrenCount);
        int childrenLocation = lua_gettop(luaState);
        for (int i = 1; i <= childrenCount; i++) {
            lua_rawgeti(luaState, childrenLocation, i);
            ret->children.push_back(readGuiLayoutNode(handlerOffset + 2));
        }
    }

    // remove the stack clutter
    lua_remove(luaState, toRemove);
    lua_remove(luaState, tableIndex);
    return ret;
}

void LuaWrapper::dumpStack() {
    printf("Lua stack dump:\n");
    int top = lua_gettop(luaState);
    for (int i = 1; i <= top; i++) {
        printf("%d\t%s\t", i, luaL_typename(luaState, i));
        switch (lua_type(luaState, i)) {
            case LUA_TNUMBER:
                printf("%g\n", lua_tonumber(luaState, i));
                break;
            case LUA_TSTRING:
                printf("%s\n", lua_tostring(luaState, i));
                break;
            case LUA_TBOOLEAN:
                printf("%s\n", (lua_toboolean(luaState, i) ? "true" : "false"));
                break;
            case LUA_TNIL:
                printf("%s\n", "nil");
                break;
            case LUA_TTABLE:
                printf("%s\n", "A table");
                break;
            default:
                printf("%p\n", lua_topointer(luaState, i));
                break;
        }
    }
}

#include "entity.hpp"

Entity *LuaWrapper::loadEntityFile(const std::string& filename) {
    std::string luaName = filename;
    const size_t delim = luaName.find_last_of("\\/");
    if (std::string::npos != delim) {
        luaName.erase(0, delim + 1);
    }
    const size_t dot = luaName.rfind('.');
    if (std::string::npos != dot) {
        luaName.erase(dot);
    }
    luaName[0] = toupper(luaName[0]);

    if (luaL_loadfile(luaState, filename.c_str()) || lua_pcall(luaState, 0, 0, 0))
        error("Could not load gui file: %s", lua_tostring(luaState, -1));

    lua_getglobal(luaState, luaName.c_str());
    if (!lua_istable(luaState, -1))
        error("%s should be a table", luaName.c_str());

    auto model = getStringField("model");
    auto texture = getStringField("texture");
    auto icon = getStringField("icon");

    luaName[0] = tolower(luaName[0]);
    auto ret = new Entity(luaName.c_str(), model.c_str(), texture.c_str(), icon.c_str());

    lua_pushstring(luaState, "weapons");
    lua_gettable(luaState, -2);
    if (!lua_isnil(luaState, -1)) {
        if (!lua_istable(luaState, -1))
            error("Weapons should be a table");
        int weaponsCount = lua_objlen(luaState, -1);

        for (int i = 1; i <= weaponsCount; i++) {
            lua_rawgeti(luaState, -1, i);
            if (!lua_istable(luaState, -1))
                error("Invalid weapon (table)");
            lua_pushstring(luaState, "name");
            lua_gettable(luaState, -2);
            if (!lua_isstring(luaState, -1))
                error("Invalid weapon (string)");

            const char *s = lua_tostring(luaState, -1);
            size_t len = lua_strlen(luaState, -1); // I guess we ignore the length for now
            lua_pop(luaState, 1);
            ret->weaponNames.push_back(std::string(s));

            std::vector<float> position;
            getNumbersField("position", position);
            if (position.size() != 3)
                error("Invalid weapon (position)");
            ret->weaponPositions.push_back({ position[0], position[1], position[2] });

            lua_pop(luaState, 1);
        }
    }
    lua_pop(luaState, 1);
    lua_pushstring(luaState, "unitAIs");
    lua_gettable(luaState, -2);
    if (!lua_isnil(luaState, -1)) {
        if (!lua_istable(luaState, -1))
            error("unitAIs should be a table");
        int unitAICount = lua_objlen(luaState, -1);

        for (int i = 1; i <= unitAICount; i++) {
            lua_rawgeti(luaState, -1, i);
            if (!lua_isstring(luaState, -1))
                error("Invalid unit ai");

            const char *s = lua_tostring(luaState, -1);
            size_t len = lua_strlen(luaState, -1); // I guess we ignore the length for now
            lua_pop(luaState, 1);
            ret->unitAINames.push_back(std::string(s));
        }
    }
    lua_pop(luaState, 1);

    ret->maxSpeed = getNumberField("maxSpeed");
    ret->maxOmega = getNumberField("maxOmega");
    ret->mass = getNumberField("mass");
    ret->thrust = getNumberField("thrust");
    ret->acceleration = ret->thrust / ret->mass;
    ret->maxHealth = getNumberField("maxHealth");
    auto minable = getBooleanField("minable");
    ret->isResource = minable;
    ret->isUnit = !minable;
    ret->resources = getNumberField("resources");
    ret->isStation = getBooleanField("stationary");
    getStringsField("buildOptions", ret->buildOptions);
    getStringField("buildIcon", ret->buildIcon);
    getNumberField("buildPower", ret->buildPower);
    getNumberField("minePower", ret->minePower);

    return ret;
}

#include "weapon.hpp"

Weapon *LuaWrapper::loadWeaponFile(const std::string& filename) {
    std::string luaName = filename;
    const size_t delim = luaName.find_last_of("\\/");
    if (std::string::npos != delim) {
        luaName.erase(0, delim + 1);
    }
    const size_t dot = luaName.rfind('.');
    if (std::string::npos != dot) {
        luaName.erase(dot);
    }
    luaName[0] = toupper(luaName[0]);

    if (luaL_loadfile(luaState, filename.c_str()) || lua_pcall(luaState, 0, 0, 0))
        error("Could not load gui file: %s", lua_tostring(luaState, -1));

    lua_getglobal(luaState, luaName.c_str());
    if (!lua_istable(luaState, -1))
        error("%s should be a table", luaName.c_str());

    WeaponKind kind = static_cast<WeaponKind>((int)getNumberField("kind"));

    std::string model, texture;
    Weapon *ret;

    switch (kind) {
        case WeaponKind::PLASMA_CANNON:
            model = getStringField("model");
            texture = getStringField("texture");

            ret = new PlasmaCannon(std::make_shared<Entity>(ENT_PROJECTILE, luaName.c_str(), model.c_str(), texture.c_str()));
            ret->entity->maxSpeed = getNumberField("speed");
            getStringField("death_sound", ret->entity->deathSound);

            ret->entity->precompute();
            break;
        case WeaponKind::BEAM:
            ret = new Beam();
            static_cast<Beam *>(ret)->color = (uint32_t)getNumberField("color");
            break;
        case WeaponKind::GUIDED:
            model = getStringField("model");
            texture = getStringField("texture");

            ret = new Guided(std::make_shared<Entity>(ENT_PROJECTILE, luaName.c_str(), model.c_str(), texture.c_str()));
            ret->entity->maxSpeed = getNumberField("speed");
            ret->entity->acceleration = getNumberField("acceleration");
            ret->entity->maxOmega = getNumberField("maxOmega");
            ret->entity->isGuided = true;
            getStringField("death_sound", ret->entity->deathSound);

            break;
        default:
            throw std::runtime_error("Lua error: unsupported weapon kind.");
    }

    luaName[0] = tolower(luaName[0]);
    ret->name = luaName;
    getNumberField("damage", ret->damage);
    getStringField("sound", ret->sound);
    getNumberField("range", ret->range);
    getNumbersField("reload", ret->reload);
    assert(ret->reload.size());

    // This does not belong here, but since I went with a trivial constructor idk where to put it
    if (kind == WeaponKind::BEAM) {
        ret->damage /= Beam::framesToFire;
    } else {
        ret->entity->precompute();
    }

    return ret;
}

void LuaWrapper::loadFile(const std::string& filename) {
    if (luaL_loadfile(luaState, filename.c_str()) || lua_pcall(luaState, 0, 0, 0))
        error("Could not load file: %s", lua_tostring(luaState, -1));
}

void LuaWrapper::doString(const char *str) {
    luaL_dostring(luaState, str);
}

GuiLayoutNode::~GuiLayoutNode() {
    for (auto child : children)
        delete child;
}

void LuaWrapper::enableCallbacksOnThisThread() {
    threadLuaInstance = this;
    lua_createtable(luaState, 0, 0);
    lua_setglobal(luaState, "Server_callbacks");
}

void LuaWrapper::enableKeyBindings() {
    isGuiThread = true;
    lua_createtable(luaState, 0, 0);
    lua_setglobal(luaState, "Keybinding_callbacks");
}

void LuaWrapper::callKeyBinding(int key, int mods) {
    lua_getglobal(luaState, "Keybinding_callbacks");
    lua_pushinteger(luaState, key);
    lua_gettable(luaState, -2);
    lua_pushinteger(luaState, mods);
    if (lua_isnil(luaState, -2)) {
        std::cerr << "Warning missing keybinding (" << std::dec << key << "). This probably indicates a lua programming error." << std::endl;
        lua_pop(luaState, 2);
        return;
    }
    if(lua_pcall(luaState, 1, 0, 0))
        error("Error running function: %s", lua_tostring(luaState, -1));
    lua_pop(luaState, 1);
}