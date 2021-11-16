#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "libs/tiny_obj_loader.h"

#include "entity.hpp"

#include <csignal>
#include <iostream>

namespace Entities {
    const std::array<unsigned char, 4> dummyTexture = {
        0x70, 0x50, 0x60, 0xff
    };

    // 0 1
    // 3 2
    // const std::array<Utilities::Vertex, 4> iconPrimativeVertices = {{
    //     { { -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f , 0.0f }, { 0.0f, 0.0f, 1.0f } },
    //     { {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f , 0.0f }, { 0.0f, 0.0f, 1.0f } },
    //     { {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f , 1.0f }, { 0.0f, 0.0f, 1.0f } },
    //     { { -1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f , 1.0f }, { 0.0f, 0.0f, 1.0f } }
    // }};

    // const std::array<uint32_t, 6> iconPrimativeIndices = {
    //     0, 1, 3, 3, 1, 2
    // };
}

Entity::Entity(SpecialEntities entityType, const char *name, const char *model, const char *texture)
: isUnit(false) {
    switch (entityType) {
        case ENT_ICON:
            textureWidth = textureHeight = 1;
            textureChannels = Entities::dummyTexture.size();
            texturePixels = (stbi_uc *)Entities::dummyTexture.data();

            // 0 1
            // 3 2
            vertices = std::vector<Utilities::Vertex>({{
                { { -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f , 0.0f }, { 0.0f, 0.0f, 1.0f } },
                { {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f , 0.0f }, { 0.0f, 0.0f, 1.0f } },
                { {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f , 1.0f }, { 0.0f, 0.0f, 1.0f } },
                { { -1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f , 1.0f }, { 0.0f, 0.0f, 1.0f } }
            }});
            indices = std::vector<uint32_t>({
                0, 1, 3, 3, 1, 2
            });

            boundingRadius = sqrtf(2);
            hasConstTexture = true;
            hasTexture = true;
            hasIcon = false;
            isProjectile = false;
            break;
        case ENT_PROJECTILE:
            this->name = std::string(name);
            hasConstTexture = false;
            hasTexture = true;
            hasIcon = false;
            isProjectile = true;
            texturePixels = stbi_load(texture, &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
            if (!texturePixels) throw std::runtime_error("Failed to load texture image.");

            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;

            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model))
                throw std::runtime_error(std::string("Unable to load model: ") + warn + " : " + err);

            std::unordered_map<Utilities::Vertex, uint32_t> uniqueVertices {};

            float d2max = 0;

            for(const auto& shape : shapes) {
                for (const auto& index : shape.mesh.indices) {
                    Utilities::Vertex vertex {};

                    vertex.pos = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    };

                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };

                    vertex.color = { 1.0f, 1.0f, 1.0f };

                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    };

                    if (uniqueVertices.count(vertex) == 0) {
                        uniqueVertices[vertex] = vertices.size();
                        vertices.push_back(vertex);
                    }

                    indices.push_back(uniqueVertices[vertex]);

                    float d2 = dot(vertex.pos, vertex.pos);
                    if (d2 > d2max) d2max = d2;
                }
            }

            boundingRadius = sqrtf(d2max);

            break;
    }
}

Entity::Entity(const char *name, const char *model, const char *texture, const char *icon)
: Entity(model, texture, icon) {
    this->name = std::string(name);
}

Entity::Entity(const char *model, const char *texture, const char *icon)
: isProjectile(false), isUnit(true) {
    hasConstTexture = false;
    if (hasTexture = strlen(texture)) {
        texturePixels = stbi_load(texture, &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
        if (!texturePixels) throw std::runtime_error("Failed to load texture image.");
    }

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model))
        throw std::runtime_error(std::string("Unable to load model: ") + warn + " : " + err);

    std::unordered_map<Utilities::Vertex, uint32_t> uniqueVertices {};

    float d2max = 0;

    for(const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Utilities::Vertex vertex {};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = vertices.size();
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);

            float d2 = dot(vertex.pos, vertex.pos);
            if (d2 > d2max) d2max = d2;
        }
    }

    boundingRadius = sqrtf(d2max);

    if (hasIcon = strlen(icon)) {
        iconPixels = stbi_load(icon, &iconWidth, &iconHeight, &iconChannels, STBI_rgb_alpha);
        if (!iconPixels) throw std::runtime_error("Failed to load icon image.");
    }
}

std::vector<uint32_t> Entity::mapOffsetToIndices(size_t offset) {
    std::vector<uint32_t> ret;
    std::transform(indices.begin(), indices.end(), std::back_inserter(ret), [offset](auto idx){ return idx + offset; });
    return ret;
}

Entity::~Entity() {
    if (hasTexture && !hasConstTexture && texturePixels) STBI_FREE(texturePixels);
    if (hasIcon && iconPixels) STBI_FREE(iconPixels);
}

void Entity::precompute() {
    dv = acceleration * Net::secondsPerTick;
    v_m = maxSpeed  * Net::secondsPerTick;
    w_m = maxOmega  * Net::secondsPerTick;

    float v = 0;
    float d = 0;
    // The lazy way, the non lazy way involves computing a bunch of cube roots for every movement update which seems worse
    // (You need to solve (n^3 + 2n^2 + 2n)/6 - D_n/dV_s = 0 which I don't see a trick to solve without a general form solution)
    do {
        brakingCurve.push_back(d);
        v += dv;
        d += v;
    } while (v < maxSpeed);
}