#pragma once

#include <string>

class Instance;
class LuaWrapper;

class UnitAI {
public:
    UnitAI(const std::string& name, LuaWrapper *lua);

    UnitAI(const UnitAI& other) = delete;
    UnitAI(UnitAI&& other) noexcept = delete;
    UnitAI& operator=(const UnitAI& other) = delete;
    UnitAI& operator=(UnitAI&& other) noexcept = delete;

    void run(Instance& instance);

    std::string name;
private:
    std::string luaName;
    LuaWrapper *lua;
};