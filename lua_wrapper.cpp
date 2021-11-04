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

void LuaWrapper::loadGuiFile(const char *name) {
    std::string filename = std::string(name) + ".lua";
    if (luaL_loadfile(luaState, filename.c_str()) || lua_pcall(luaState, 0, 0, 0))
        error("Could not load gui file: %s", lua_tostring(luaState, -1));
    
    lua_getglobal(luaState, name);
    if (!lua_isnumber(luaState, -1))
        error("%s should be a table\n", name);
    
    // int hud = (int)lua_tonumber(luaState, -1);
    // std::cout << "Lau said that it is " << hud << std::endl;

    lua_pop(luaState, 1);
}