#include "engine.hpp"

#include <filesystem>

std::vector<Entity *> Scene::loadEntitiesFromLua(const char *directory) {
    LuaWrapper lua;
    std::vector<Entity *> ret;

        for (const auto & entry : std::filesystem::directory_iterator(directory)) {
            try {
                ret.push_back(lua.loadEntityFile(entry.path()));
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

Scene::~Scene() {
    for (auto& [name, ent] : entities) {
        delete ent;
    }
}