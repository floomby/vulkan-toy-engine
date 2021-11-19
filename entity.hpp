#pragma once

#include "net.hpp"
#include "unit_ai.hpp"
#include "utilities.hpp"
#include "weapon.hpp"

typedef unsigned char stbi_uc;

enum SpecialEntities {
    ENT_ICON,
    ENT_PROJECTILE
};

class UnitAI;

// TODO It would seem good to support empty entities (ie. no model)
class Entity {
    friend class EntityTexture;
    friend class Engine;
    friend class Scene;
public:
    Entity(SpecialEntities entityType, const char *name = "", const char *model = "", const char *texture = "");
    Entity(const char *model, const char *texture = "", const char *icon = "");
    Entity(const char *name, const char *model, const char *texture, const char *icon);
    std::vector<uint32_t> mapOffsetToIndices(size_t offset);

    Entity(const Entity& other) = delete;
    Entity(Entity&& other) noexcept = delete;
    Entity& operator=(const Entity& other) = delete;
    Entity& operator=(Entity&& other) noexcept = delete;
    ~Entity();
    // temp stuff
    float boundingRadius;
    int textureIndex, iconIndex, modelIndex;

    float maxSpeed = 0.5f;
    float acceleration = 0.1f;
    // maybe this should be a glm::vec3 for yaw, pitch and roll seperately
    float maxOmega = 0.1f;
    std::string name;

    float dv;
    float v_m;
    float w_m;

    bool isProjectile, isUnit;
    WeaponKind weaponKind;

    std::vector<std::string> weaponNames;
    std::vector<Weapon *> weapons;

    std::vector<std::string> unitAINames;
    std::vector<UnitAI *> ais;

    void precompute();
    inline float indexToVelocity(uint idx) {
        return float(idx * (idx + 1) >> 1) * dv;
    }
    inline float velocityToIndex(float velocity) {
        return -1.f + sqrtf(velocity / dv * 2.f + 1.f);
    }
    std::vector<float> brakingCurve; // we can just precompute this
protected:
    stbi_uc *texturePixels;
    stbi_uc *iconPixels;
    int textureWidth, textureHeight, textureChannels;
    int iconWidth, iconHeight, iconChannels;
    std::vector<Utilities::Vertex> vertices;
    std::vector<uint32_t> indices;
    bool hasIcon, hasTexture;
private:
    bool hasConstTexture;
};