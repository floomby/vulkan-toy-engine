#include "lua_wrapper.hpp"

#include <iostream>

LuaWrapper::LuaWrapper() {
    luaState = luaL_newstate();
    luaL_openlibs(luaState);
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

float LuaWrapper::getNumberField(const char *key) {
    float result;
    lua_pushstring(luaState, key);
    lua_gettable(luaState, -2);
    if (!lua_isnumber(luaState, -1))
        error("Invalid number for field %s", key);
    result = lua_tonumber(luaState, -1);
    lua_pop(luaState, 1);
    return result;
}

// put it on the top of the stack
bool LuaWrapper::getFunctionField(const char *key) {
    lua_pushstring(luaState, key);
    lua_gettable(luaState, -2);
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
        error("Invalid number for field %s", key);
    const char *s = lua_tostring(luaState, -1);
    size_t len = lua_strlen(luaState, -1); // I guess we ignore the length for now
    lua_pop(luaState, 1);
    return std::string(s);
}


GuiLayoutNode *LuaWrapper::loadGuiFile(const char *name) {
    std::string filename = std::string(name);
    std::transform(filename.begin(), filename.end(), filename.begin(), [](unsigned char c) { return std::tolower(c); });
    filename += ".lua";
    if (luaL_loadfile(luaState, filename.c_str()) || lua_pcall(luaState, 0, 0, 0))
        error("Could not load gui file: %s", lua_tostring(luaState, -1));
    
    lua_getglobal(luaState, name);
    if (!lua_istable(luaState, -1))
        error("%s should be a table\n", name);
    
    auto ret = readGuiLayoutNode();

    // if(lua_pcall(luaState, 0, 0, 0))
    //     error("Error running function: %s", lua_tostring(luaState, -1));

    return ret;
    // lua_pop(luaState, 1);
}

GuiLayoutNode *LuaWrapper::readGuiLayoutNode() {
    auto ret = new GuiLayoutNode;
    ret->x = getNumberField("x");
    ret->y = getNumberField("y");
    ret->height = getNumberField("height");
    ret->width = getNumberField("width");
    ret->kind = (GuiLayoutType)(int)getNumberField("kind");
    if (ret->kind == GuiLayoutType::TEXT_BUTTON) {
        ret->text = getStringField("text");
    }
    if (getFunctionField("onClick")) {
        ret->handlers.insert({ "onClick", lua_gettop(luaState) });
    }
    return ret;
}

void LuaWrapper::callFunction(int index) {
    lua_pushvalue(luaState, index);
    if(lua_pcall(luaState, 0, 0, 0))
        error("Error running function: %s", lua_tostring(luaState, -1));
}