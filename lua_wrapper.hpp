#pragma once

#include <luajit-2.1/lua.hpp>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

template<typename E, E V> constexpr std::string getEnumString() {
    auto name = __PRETTY_FUNCTION__;
    int i = strlen(name), j;
    for (int spaces = 0; i >= 0; --i) {
        if (name[i] == ' ') {
            spaces++;
            if (spaces == 3) j = i;
            if (spaces == 4) break;
        }
    }
    i++;
    std::string ret = "";
    for (; i < j-1; ++i) ret += name[i];
    return ret;
}

template<typename E, int val> constexpr std::vector<std::string> enumNames_i(std::vector<std::string> ret) {
    ret.push_back(getEnumString<E, E(val)>());
    if constexpr (val == 0)
        return ret;
    else
        return enumNames_i<E, val - 1>(ret);
}

// Last enum value must be COUNT
template<typename E> constexpr std::vector<std::string> enumNames() {
    std::vector<std::string> ret;
    return enumNames_i<E, static_cast<int>(E::COUNT) - 1>(ret);
}

enum class GuiLayoutType {
    PANEL,
    TEXT_BUTTON,
    COUNT
};

struct GuiLayoutNode {
    float x, y, height, width;
    GuiLayoutType kind;
    // std::vector<GuiLayoutNode *> children;
    std::map<std::string, int> handlers;
    std::string text;
};

#include <iostream>

class LuaWrapper {
public:
    LuaWrapper();
    ~LuaWrapper();

    template<typename E> void exportEnumToLua() {
        auto names = enumNames<E>();
        std::reverse(names.begin(), names.end());
        int value = 0;
        for (auto& name : names) {
            std::replace(name.begin(), name.end(), ':', '_');
            lua_pushnumber(luaState, value);
            lua_setglobal(luaState, name.c_str());
            value++;
        }
    }

    GuiLayoutNode *loadGuiFile(const char *name);
    void callFunction(int index);
private:
    void error(const char *fmt, ...);
    float getNumberField(const char *key);
    bool getFunctionField(const char *key);
    std::string getStringField(const char *key);
    GuiLayoutNode *readGuiLayoutNode();
    lua_State *luaState;
};