#pragma once

#include "utilities.hpp"

typedef unsigned char stbi_uc;

enum SpecialEntities {
    ENT_ICON,
};

// TODO It would seem good to support empty entities (ie. no model)
class Entity {
    friend class InternalTexture;
    friend class Engine;
    friend class Scene;
public:
    Entity(SpecialEntities entityType);
    Entity(const char *model, const char *texture = "", const char *icon = "");
    Entity(const char *name, const char *model, const char *texture, const char *icon);
    std::vector<uint32_t> mapOffsetToIndices(size_t offset);

    Entity(const Entity& other);
    Entity(Entity&& other) noexcept;
    Entity& operator=(const Entity& other);
    Entity& operator=(Entity&& other) noexcept;
    ~Entity();
    // temp stuff
    Entity translate(glm::vec3 delta);
    float boundingRadius;
    int textureIndex, iconIndex, modelIndex;

    float maxSpeed = 0.5f;
    float acceleration = 0.1f;
    // maybe this should be a glm::vec3 for yaw, pitch and roll seperately
    float dOmega = 0.1f;
    std::string name;
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