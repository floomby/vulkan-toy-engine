#pragma once

#include "utilities.hpp"

typedef unsigned char stbi_uc;

// TODO It would seem good to support empty entities (ie. no model)
class Entity : Utilities {
    friend class InternalTexture;
    friend class Engine;
    friend class Scene;
public:
    Entity(const char *model, const char *texture);
    std::vector<uint32_t> mapOffsetToIndices(size_t offset);

    Entity(const Entity& other);
    Entity(Entity&& other) noexcept;
    Entity& operator=(const Entity& other);
    Entity& operator=(Entity&& other) noexcept;
    ~Entity();
    // temp stuff
    Entity translate(glm::vec3 delta);
protected:
    stbi_uc *texturePixels;
    int textureWidth, texureHeight, texureChannels;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};