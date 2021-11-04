#pragma once

#include <luajit-2.1/lua.hpp>

#include <string>
#include <vector>

template<typename E, E V> constexpr std::string getEnumString() {
    auto name = __PRETTY_FUNCTION__;
    int i = strlen(name);
    int n = i, j;
    // Find the final space character in the pretty name.
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
    return enumNames_i<E, static_cast<int>(E::COUNT) - 1>(ret).reverse();
}

class LuaWrapper {
public:
    LuaWrapper();
    ~LuaWrapper();

    template<typename E> void exportEnumToLua() {
        auto names = enumNames<E>();
        
    }

    void loadGuiFile(const char *name);
    void error(const char *fmt, ...);
private:
    lua_State *luaState;
};