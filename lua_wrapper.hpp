#pragma once

#include <luajit-2.1/lua.hpp>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

template<typename E, E I> constexpr bool isValidEnumIndex() {
    auto name = __PRETTY_FUNCTION__;
    int i = strlen(name);
    for (; i >= 0; --i) {
        if (name[i] == ' ') return true;
        if (name[i] == ')') return false;
    }
    return false;
}

template<typename E, int I> constexpr int getEnumCount() {
    if constexpr (isValidEnumIndex<E, static_cast<E>(I)>()) return getEnumCount<E, I + 1>();
    else return I + 1;
}

template<typename E> constexpr int getEnumCount() {
    return getEnumCount<E, 0>();
}

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

template<typename E> constexpr std::vector<std::string> enumNames() {
    std::vector<std::string> ret;
    return enumNames_i<E, static_cast<int>(getEnumCount<E>()) - 1>(ret);
}

enum class GuiLayoutKind {
    PANEL,
    TEXT_BUTTON,
    IMAGE_BUTTON
};

struct GuiLayoutNode {
    float x, y, height, width;
    GuiLayoutKind kind;
    std::vector<GuiLayoutNode *> children;
    std::map<std::string, int> handlers;
    std::string text;   // I guess do this like imageStates as well
    uint32_t color;
    std::string name;
    std::vector<std::string> imageStates;
};

#include <iostream>

class Entity;
class Weapon;
class UnitAI;

class LuaWrapper {
    friend class UnitAI;
public:
    LuaWrapper(bool rethrowExceptions = false);
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
    Entity *loadEntityFile(const std::string& filename);
    Weapon *loadWeaponFile(const std::string& filename);

    inline void callFunction(int index) {
        callFunction<0>(index);
    }

    template<int argc>
    void callFunction(int index) {
        lua_pushvalue(luaState, index);
        lua_insert(luaState, -1 - argc);
        if(lua_pcall(luaState, argc, 0, 0))
            error("Error running function: %s", lua_tostring(luaState, -1));
    }

    void callFunction(const std::string& name) {
        callFunction<0>(name);
    }

    template<int argc>
    void callFunction(const std::string& name) {
        throw std::runtime_error("not implemented");
    }

private:
    template<typename T>
    void stackPusher(const T& arg) {
        if constexpr (std::is_arithmetic<T>::value) {
            lua_pushnumber(luaState, arg);
        }
        if constexpr (std::is_enum<T>::value) {
            lua_pushnumber(luaState, static_cast<int>(arg));
        }
        static_assert(std::is_arithmetic<T>::value || std::is_enum<T>::value, "Unsupported type");
    }

    template<int argc, typename N, typename T>
    void callFunction(const N& n, const T& arg) {
        stackPusher(arg);
        callFunction<argc + 1>(n);
    }

    template<int argc, typename N, typename T, typename... Args>
    void callFunction(const N& n, const T& arg, Args... args) {
        stackPusher(arg);
        callFunction<argc + 1>(n, arg);
    }
public:

    template<typename N, typename T>
    void callFunction(const N& n, const T& arg) {
        callFunction<0>(n, arg);
    }

    template<typename N, typename T, typename... Args>
    void callFunction(const N& n, const T& arg, Args... args) {
        callFunction<0>(n, arg, args...);
    }


    void apiExport();
    void loadFile(const std::string& filename);

    void doString(const char *str);
private:
    void error(const char *fmt, ...);
    double getNumberField(const char *key);
    bool getFunctionField(const char *key, int pos = 0);
    std::string getStringField(const char *key);
    void getStringField(const char *key, std::string& str);
    void getStringsField(const char *key, std::vector<std::string>& strs);
    GuiLayoutNode *readGuiLayoutNode(int handlerOffset = 0);
    lua_State *luaState;

    void dumpStack();
};