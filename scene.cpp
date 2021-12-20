#include "engine.hpp"
#include "unit_ai.hpp"
#include "weapon.hpp"

#include <filesystem>

InstanceZSorter::InstanceZSorter(Scene *context)
: context(context) {}

bool InstanceZSorter::operator() (int a, int b) {
    return context->state.instances[a]->cammeraDistance2 > context->state.instances[b]->cammeraDistance2;
}

std::vector<Entity *> Scene::loadEntitiesFromLua(const char *directory) {
    std::vector<Entity *> ret;

    for (const auto & entry : std::filesystem::directory_iterator(directory)) {
        try {
            if (entry.path().string().find(".lua") == entry.path().string().length() - 4) {
                ret.push_back(context->lua->loadEntityFile(entry.path()));
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
    context->lua->exportEnumToLua<WeaponKind>();

    std::vector<Weapon *> ret;

    for (const auto & entry : std::filesystem::directory_iterator(directory)) {
        try {
            if (entry.path().string().find(".lua") == entry.path().string().length() - 4) {
                ret.push_back(context->lua->loadWeaponFile(entry.path()));
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

void Scene::initUnitAIs(LuaWrapper *lua, const char* directory) {
    for (const auto & entry : std::filesystem::directory_iterator(directory)) {
        try {
            if (entry.path().string().find(".lua") == entry.path().string().length() - 4) {
                lua->loadFile(entry.path());
                this->ais.insert({ entry.path().stem(), new UnitAI(entry.path().stem(), lua) });
            }
        } catch (const std::exception& e) {
            std::string msg = "Error loading unit ai (";
            msg += entry.path();
            msg += "): ";
            msg += e.what();
            throw std::runtime_error(msg);
        }
    }

    for (auto& [ename, ent] : this->entities) {
        for (const auto& name : ent->unitAINames) {
            if (!ais.contains(name)) {
                std::cerr << "Missing unit ai " << name << " for entity " << ename << std::endl;
                continue;
            }
            ent->ais.push_back(ais[name]);
        }
    }
}

Scene::~Scene() {
    for (auto& [name, ent] : entities) {
        // Projectile entities are managed by the weapon class they belong with (they are also a shared ptr anyways)
        if (!ent->isProjectile) delete ent;
    }

    for (auto& [name, weapon] : weapons) {
        delete weapon;
    }

    for (auto& [name, ai] : ais) {
        delete ai;
    }
}

#include "server.hpp"

Scene::Scene(Server *context)
: context(context), zSorter(InstanceZSorter(this)) {
    auto weapons = loadWeaponsFromLua("weapons");
    for (const auto& weapon : weapons) {
        if (weapon->hasEntity()) {
            this->entities.insert({ weapon->entity->name, weapon->entity.get() });
        }
        this->weapons.insert({ weapon->name, weapon });
    }
    auto ents = loadEntitiesFromLua("units");
    for (const auto& entity : ents) {
        this->entities.insert({ entity->name, entity });
        // Populate the weapon vector
        for (auto& weapon : entity->weaponNames) {
            if (this->weapons.contains(weapon)) {
                entity->weapons.push_back(this->weapons[weapon]);
            } else {
                std::cerr << "Weapon not found: " << weapon << std::endl;
            }
        }
    }
}