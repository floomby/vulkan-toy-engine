#include "engine.hpp"
#include "unit_ai.hpp"
#include "weapon.hpp"

#include <filesystem>

std::vector<Entity *> Scene::loadEntitiesFromLua(const char *directory) {
    LuaWrapper lua;
    std::vector<Entity *> ret;

    for (const auto & entry : std::filesystem::directory_iterator(directory)) {
        try {
            if (entry.path().string().find(".lua") == entry.path().string().length() - 4) {
                ret.push_back(lua.loadEntityFile(entry.path()));
            }
        } catch (const std::exception& e) {
            std::string msg = "Error reading unit (";
            msg += entry.path();
            msg += "): ";
            msg += e.what();
            throw std::runtime_error(msg);
        }
    }

    return ret;
}

std::vector<Weapon *> Scene::loadWeaponsFromLua(const char *directory) {
    LuaWrapper lua;
    lua.exportEnumToLua<WeaponKind>();
    std::vector<Weapon *> ret;

    for (const auto & entry : std::filesystem::directory_iterator(directory)) {
        try {
            if (entry.path().string().find(".lua") == entry.path().string().length() - 4) {
                ret.push_back(lua.loadWeaponFile(entry.path()));
            }
        } catch (const std::exception& e) {
            std::string msg = "Error reading weapon (";
            msg += entry.path();
            msg += "): ";
            msg += e.what();
            throw std::runtime_error(msg);
        }
    }

    return ret;
}

Scene::~Scene() {
    for (auto& [name, ent] : entities) {
        delete ent->ai;
        delete ent;
    }

    for (auto& [name, weapon] : weapons) {
        delete weapon;
    }
}