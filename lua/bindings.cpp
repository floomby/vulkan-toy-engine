// This is an auto generated file changes can be overwritten durring build process
#include "../lua_wrapper.hpp"
#include "../api.hpp"

static int cmd_moveWrapper(lua_State *ls) {
    auto a0 = (InstanceID)luaL_checkinteger(ls, 1);
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
    auto a2 = (InsertionMode)luaL_checkinteger(ls, 3);
    Api::cmd_move(a0, a1, a2);
    return 0;
}

static void cmd_moveExport(lua_State *ls) {
    lua_pushcfunction(ls, cmd_moveWrapper);
    lua_setglobal(ls, "cmd_move");
}

static int cmd_setTargetIDWrapper(lua_State *ls) {
    auto a0 = (InstanceID)luaL_checkinteger(ls, 1);
    auto a1 = (InstanceID)luaL_checkinteger(ls, 2);
    auto a2 = (InsertionMode)luaL_checkinteger(ls, 3);
    Api::cmd_setTargetID(a0, a1, a2);
    return 0;
}

static void cmd_setTargetIDExport(lua_State *ls) {
    lua_pushcfunction(ls, cmd_setTargetIDWrapper);
    lua_setglobal(ls, "cmd_setTargetID");
}

static int cmd_createInstanceWrapper(lua_State *ls) {
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
    auto a3 = (TeamID)luaL_checkinteger(ls, 4);
    Api::cmd_createInstance(a0, a1, a2, a3);
    return 0;
}

static void cmd_createInstanceExport(lua_State *ls) {
    lua_pushcfunction(ls, cmd_createInstanceWrapper);
    lua_setglobal(ls, "cmd_createInstance");
}

static int cmd_stopWrapper(lua_State *ls) {
    auto a0 = (InstanceID)luaL_checkinteger(ls, 1);
    auto a1 = (InsertionMode)luaL_checkinteger(ls, 2);
    Api::cmd_stop(a0, a1);
    return 0;
}

static void cmd_stopExport(lua_State *ls) {
    lua_pushcfunction(ls, cmd_stopWrapper);
    lua_setglobal(ls, "cmd_stop");
}

static int cmd_destroyInstanceWrapper(lua_State *ls) {
    auto a0 = (InstanceID)luaL_checkinteger(ls, 1);
    Api::cmd_destroyInstance(a0);
    return 0;
}

static void cmd_destroyInstanceExport(lua_State *ls) {
    lua_pushcfunction(ls, cmd_destroyInstanceWrapper);
    lua_setglobal(ls, "cmd_destroyInstance");
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
    auto a3 = (uint32_t)luaL_checkinteger(ls, 4);
    Api::eng_createBallisticProjectile(a0, a1, a2, a3);
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

static int eng_getTeamIDWrapper(lua_State *ls) {
    auto a0 = (InstanceID)luaL_checkinteger(ls, 1);
    auto r = Api::eng_getTeamID(a0);
    lua_pushinteger(ls, r);    return 1;
}

static void eng_getTeamIDExport(lua_State *ls) {
    lua_pushcfunction(ls, eng_getTeamIDWrapper);
    lua_setglobal(ls, "eng_getTeamID");
}

static int eng_getSelectedInstancesWrapper(lua_State *ls) {
    auto r = Api::eng_getSelectedInstances();
    lua_createtable(ls, r.size(), 0);
    for (int i = 0; i < r.size(); i++) {
        lua_pushinteger(ls, r[i]);
        lua_rawseti(ls, -2, i + 1);
    }
    return 1;
}

static void eng_getSelectedInstancesExport(lua_State *ls) {
    lua_pushcfunction(ls, eng_getSelectedInstancesWrapper);
    lua_setglobal(ls, "eng_getSelectedInstances");
}

static int eng_setInstanceStateEngageWrapper(lua_State *ls) {
    auto a0 = (InstanceID)luaL_checkinteger(ls, 1);
    auto a1 = (IEngage)luaL_checkinteger(ls, 2);
    Api::eng_setInstanceStateEngage(a0, a1);
    return 0;
}

static void eng_setInstanceStateEngageExport(lua_State *ls) {
    lua_pushcfunction(ls, eng_setInstanceStateEngageWrapper);
    lua_setglobal(ls, "eng_setInstanceStateEngage");
}

static int eng_setInstanceHealthWrapper(lua_State *ls) {
    auto a0 = (InstanceID)luaL_checkinteger(ls, 1);
    auto a1 = (float)luaL_checknumber(ls, 2);
    Api::eng_setInstanceHealth(a0, a1);
    return 0;
}

static void eng_setInstanceHealthExport(lua_State *ls) {
    lua_pushcfunction(ls, eng_setInstanceHealthWrapper);
    lua_setglobal(ls, "eng_setInstanceHealth");
}

static int eng_getInstanceHealthWrapper(lua_State *ls) {
    auto a0 = (InstanceID)luaL_checkinteger(ls, 1);
    auto r = Api::eng_getInstanceHealth(a0);
    lua_pushnumber(ls, r);    return 1;
}

static void eng_getInstanceHealthExport(lua_State *ls) {
    lua_pushcfunction(ls, eng_getInstanceHealthWrapper);
    lua_setglobal(ls, "eng_getInstanceHealth");
}

static int eng_getInstanceEntityNameWrapper(lua_State *ls) {
    auto a0 = (InstanceID)luaL_checkinteger(ls, 1);
    auto r = Api::eng_getInstanceEntityName(a0);
    lua_pushstring(ls, r.c_str());    return 1;
}

static void eng_getInstanceEntityNameExport(lua_State *ls) {
    lua_pushcfunction(ls, eng_getInstanceEntityNameWrapper);
    lua_setglobal(ls, "eng_getInstanceEntityName");
}

static int eng_quitWrapper(lua_State *ls) {
    Api::eng_quit();
    return 0;
}

static void eng_quitExport(lua_State *ls) {
    lua_pushcfunction(ls, eng_quitWrapper);
    lua_setglobal(ls, "eng_quit");
}

static int gui_setVisibilityWrapper(lua_State *ls) {
    luaL_checkstring(ls, 1);
    auto a0 = lua_tostring(ls, 1);
    auto a1 = (bool)luaL_checkinteger(ls, 2);
    Api::gui_setVisibility(a0, a1);
    return 0;
}

static void gui_setVisibilityExport(lua_State *ls) {
    lua_pushcfunction(ls, gui_setVisibilityWrapper);
    lua_setglobal(ls, "gui_setVisibility");
}

static int gui_setLabelTextWrapper(lua_State *ls) {
    luaL_checkstring(ls, 1);
    auto a0 = lua_tostring(ls, 1);
    luaL_checkstring(ls, 2);
    auto a1 = lua_tostring(ls, 2);
    Api::gui_setLabelText(a0, a1);
    return 0;
}

static void gui_setLabelTextExport(lua_State *ls) {
    lua_pushcfunction(ls, gui_setLabelTextWrapper);
    lua_setglobal(ls, "gui_setLabelText");
}

static int state_dumpAuthStateIDsWrapper(lua_State *ls) {
    Api::state_dumpAuthStateIDs();
    return 0;
}

static void state_dumpAuthStateIDsExport(lua_State *ls) {
    lua_pushcfunction(ls, state_dumpAuthStateIDsWrapper);
    lua_setglobal(ls, "state_dumpAuthStateIDs");
}

static int state_giveResourcesWrapper(lua_State *ls) {
    auto a0 = (TeamID)luaL_checkinteger(ls, 1);
    auto a1 = (double)luaL_checknumber(ls, 2);
    Api::state_giveResources(a0, a1);
    return 0;
}

static void state_giveResourcesExport(lua_State *ls) {
    lua_pushcfunction(ls, state_giveResourcesWrapper);
    lua_setglobal(ls, "state_giveResources");
}

static int state_getResourcesWrapper(lua_State *ls) {
    auto a0 = (TeamID)luaL_checkinteger(ls, 1);
    auto r = Api::state_getResources(a0);
    lua_pushnumber(ls, r);    return 1;
}

static void state_getResourcesExport(lua_State *ls) {
    lua_pushcfunction(ls, state_getResourcesWrapper);
    lua_setglobal(ls, "state_getResources");
}

static int net_declareTeamWrapper(lua_State *ls) {
    auto a0 = (TeamID)luaL_checkinteger(ls, 1);
    luaL_checkstring(ls, 2);
    auto a1 = lua_tostring(ls, 2);
    Api::net_declareTeam(a0, a1);
    return 0;
}

static void net_declareTeamExport(lua_State *ls) {
    lua_pushcfunction(ls, net_declareTeamWrapper);
    lua_setglobal(ls, "net_declareTeam");
}

static int net_pauseWrapper(lua_State *ls) {
    auto a0 = (bool)luaL_checkinteger(ls, 1);
    Api::net_pause(a0);
    return 0;
}

static void net_pauseExport(lua_State *ls) {
    lua_pushcfunction(ls, net_pauseWrapper);
    lua_setglobal(ls, "net_pause");
}

void LuaWrapper::apiExport() {
    cmd_moveExport(luaState);
    cmd_setTargetIDExport(luaState);
    cmd_createInstanceExport(luaState);
    cmd_stopExport(luaState);
    cmd_destroyInstanceExport(luaState);
    eng_createBallisticProjectileExport(luaState);
    eng_echoExport(luaState);
    eng_getTeamIDExport(luaState);
    eng_getSelectedInstancesExport(luaState);
    eng_setInstanceStateEngageExport(luaState);
    eng_setInstanceHealthExport(luaState);
    eng_getInstanceHealthExport(luaState);
    eng_getInstanceEntityNameExport(luaState);
    eng_quitExport(luaState);
    gui_setVisibilityExport(luaState);
    gui_setLabelTextExport(luaState);
    state_dumpAuthStateIDsExport(luaState);
    state_giveResourcesExport(luaState);
    state_getResourcesExport(luaState);
    net_declareTeamExport(luaState);
    net_pauseExport(luaState);
}
