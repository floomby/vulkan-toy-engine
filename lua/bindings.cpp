// This is an auto generated file changes can be overwritten durring build process
#include "../lua_wrapper.hpp"
#include "../api.hpp"

static int cmd_moveWrapper(lua_State *ls) {
    luaL_checkinteger(ls, 1);
    auto a0 = (uint32_t)lua_tointeger(ls, 1);
    if(!lua_istable(ls, 2)) throw std::runtime_error("Invalid lua arguments (table)");
    std::array<float, 3> v1;
    if (lua_objlen(ls, 2) != 3) throw std::runtime_error("C++/Lua vector mismatch");
    for (int i = 1; i <= 3; i++) {
        lua_rawgeti(ls, 2, i);
        luaL_checknumber(ls, -1);
        v1[i - 1] = lua_tonumber(ls, -1);
        lua_pop(ls, 1);
    }
    glm::vec3 a1(v1[0], v1[1], v1[2]);
    luaL_checkinteger(ls, 3);
    auto a2 = (InsertionMode)lua_tointeger(ls, 3);
    Api::cmd_move(a0, a1, a2);
    return 0;
}

static void cmd_moveExport(lua_State *ls) {
    lua_pushcfunction(ls, cmd_moveWrapper);
    lua_setglobal(ls, "cmd_move");
}

static int cmd_stopWrapper(lua_State *ls) {
    luaL_checkinteger(ls, 1);
    auto a0 = (uint32_t)lua_tointeger(ls, 1);
    luaL_checkinteger(ls, 2);
    auto a1 = (InsertionMode)lua_tointeger(ls, 2);
    Api::cmd_stop(a0, a1);
    return 0;
}

static void cmd_stopExport(lua_State *ls) {
    lua_pushcfunction(ls, cmd_stopWrapper);
    lua_setglobal(ls, "cmd_stop");
}

static int eng_createInstanceWrapper(lua_State *ls) {
    luaL_checkstring(ls, 1);
    auto a0 = lua_tostring(ls, 1);
    if(!lua_istable(ls, 2)) throw std::runtime_error("Invalid lua arguments (table)");
    std::array<float, 3> v1;
    if (lua_objlen(ls, 2) != 3) throw std::runtime_error("C++/Lua vector mismatch");
    for (int i = 1; i <= 3; i++) {
        lua_rawgeti(ls, 2, i);
        luaL_checknumber(ls, -1);
        v1[i - 1] = lua_tonumber(ls, -1);
        lua_pop(ls, 1);
    }
    glm::vec3 a1(v1[0], v1[1], v1[2]);
    if(!lua_istable(ls, 3)) throw std::runtime_error("Invalid lua arguments (table)");
    std::array<float, 4> v2;
    if (lua_objlen(ls, 3) != 4) throw std::runtime_error("C++/Lua vector mismatch");
    for (int i = 1; i <= 4; i++) {
        lua_rawgeti(ls, 3, i);
        luaL_checknumber(ls, -1);
        v2[i - 1] = lua_tonumber(ls, -1);
        lua_pop(ls, 1);
    }
    glm::quat a2(v2[0], v2[1], v2[2], v2[3]);
    luaL_checkinteger(ls, 4);
    auto a3 = lua_tointeger(ls, 4);
    Api::eng_createInstance(a0, a1, a2, a3);
    return 0;
}

static void eng_createInstanceExport(lua_State *ls) {
    lua_pushcfunction(ls, eng_createInstanceWrapper);
    lua_setglobal(ls, "eng_createInstance");
}

static int eng_createBallisticProjectileWrapper(lua_State *ls) {
    if (!lua_islightuserdata(ls, 1)) throw std::runtime_error("Invalid lua arguments (pointer)");
    auto a0 = (Entity*)lua_topointer(ls, 1);
    if(!lua_istable(ls, 2)) throw std::runtime_error("Invalid lua arguments (table)");
    std::array<float, 3> v1;
    if (lua_objlen(ls, 2) != 3) throw std::runtime_error("C++/Lua vector mismatch");
    for (int i = 1; i <= 3; i++) {
        lua_rawgeti(ls, 2, i);
        luaL_checknumber(ls, -1);
        v1[i - 1] = lua_tonumber(ls, -1);
        lua_pop(ls, 1);
    }
    glm::vec3 a1(v1[0], v1[1], v1[2]);
    if(!lua_istable(ls, 3)) throw std::runtime_error("Invalid lua arguments (table)");
    std::array<float, 3> v2;
    if (lua_objlen(ls, 3) != 3) throw std::runtime_error("C++/Lua vector mismatch");
    for (int i = 1; i <= 3; i++) {
        lua_rawgeti(ls, 3, i);
        luaL_checknumber(ls, -1);
        v2[i - 1] = lua_tonumber(ls, -1);
        lua_pop(ls, 1);
    }
    glm::vec3 a2(v2[0], v2[1], v2[2]);
    Api::eng_createBallisticProjectile(a0, a1, a2);
    return 0;
}

static void eng_createBallisticProjectileExport(lua_State *ls) {
    lua_pushcfunction(ls, eng_createBallisticProjectileWrapper);
    lua_setglobal(ls, "eng_createBallisticProjectile");
}

static int eng_echoWrapper(lua_State *ls) {
    luaL_checkstring(ls, 1);
    auto a0 = lua_tostring(ls, 1);
    Api::eng_echo(a0);
    return 0;
}

static void eng_echoExport(lua_State *ls) {
    lua_pushcfunction(ls, eng_echoWrapper);
    lua_setglobal(ls, "eng_echo");
}

static int test_fireWrapper(lua_State *ls) {
    Api::test_fire();
    return 0;
}

static void test_fireExport(lua_State *ls) {
    lua_pushcfunction(ls, test_fireWrapper);
    lua_setglobal(ls, "test_fire");
}

static int cmd_setTargetLocationWrapper(lua_State *ls) {
    if (!lua_islightuserdata(ls, 1)) throw std::runtime_error("Invalid lua arguments (pointer)");
    auto a0 = (Instance*)lua_topointer(ls, 1);
    if(!lua_istable(ls, 2)) throw std::runtime_error("Invalid lua arguments (table)");
    std::array<float, 3> v1;
    if (lua_objlen(ls, 2) != 3) throw std::runtime_error("C++/Lua vector mismatch");
    for (int i = 1; i <= 3; i++) {
        lua_rawgeti(ls, 2, i);
        luaL_checknumber(ls, -1);
        v1[i - 1] = lua_tonumber(ls, -1);
        lua_pop(ls, 1);
    }
    glm::vec3 a1(v1[0], v1[1], v1[2]);
    Api::cmd_setTargetLocation(a0, a1);
    return 0;
}

static void cmd_setTargetLocationExport(lua_State *ls) {
    lua_pushcfunction(ls, cmd_setTargetLocationWrapper);
    lua_setglobal(ls, "cmd_setTargetLocation");
}

void LuaWrapper::apiExport() {
    cmd_moveExport(luaState);
    cmd_stopExport(luaState);
    eng_createInstanceExport(luaState);
    eng_createBallisticProjectileExport(luaState);
    eng_echoExport(luaState);
    test_fireExport(luaState);
    cmd_setTargetLocationExport(luaState);
}
