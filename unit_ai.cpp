#include "instance.hpp"
#include "unit_ai.hpp"
#include "lua_wrapper.hpp"

UnitAI::UnitAI(const std::string& name, LuaWrapper *lua) 
: luaName(name), name(name), lua(lua) {
    this->luaName[0] = toupper(name[0]);
}

void UnitAI::run(Instance& instance) {
    lua_getglobal(lua->luaState, luaName.c_str());
    lua_pushlightuserdata(lua->luaState, &instance);
    if (lua_pcall(lua->luaState, 1, 1, 0) != 0)
        lua->error("Error running unit ai %s : %s", name.c_str(), lua_tostring(lua->luaState, -1));
}