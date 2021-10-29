#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "libs/tiny_obj_loader.h"

#include "entity.hpp"

#include <csignal>
#include <iostream>

namespace Entities {
    const std::array<unsigned char, 4> dummyTexture = {
        0x70, 0x50, 0xff, 0xff
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
};

Entity::Entity(SpecialEntities entityType) {
    textureWidth = texureHeight = 1;
    texureChannels = Entities::dummyTexture.size();
    texturePixels = (stbi_uc *)Entities::dummyTexture.data();

    // 0 1
    // 3 2
    vertices = std::vector<Vertex>({{
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
}

Entity::Entity(const char *model, const char *texture) {
    texturePixels = stbi_load(texture, &textureWidth, &texureHeight, &texureChannels, STBI_rgb_alpha);

    if (!texturePixels) throw std::runtime_error("Failed to load texture image.");

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model)) throw std::runtime_error(std::string("Unable to load model: ") + warn + err);

    std::unordered_map<Vertex, uint32_t> uniqueVertices {};

    float d2max = 0;

    for(const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex {};

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
    hasConstTexture = false;
}

std::vector<uint32_t> Entity::mapOffsetToIndices(size_t offset) {
    std::vector<uint32_t> ret;
    std::transform(indices.begin(), indices.end(), std::back_inserter(ret), [offset](auto idx){ return idx + offset; });
    return ret;
}

Entity::Entity(const Entity& other) {
    textureWidth = other.textureWidth;
    texureHeight = other.texureHeight;
    texureChannels = other.texureChannels;
    hasConstTexture = other.hasConstTexture;
    vertices = std::move(other.vertices);
    indices = std::move(other.indices);
    // I am not sure about this code (we may need to check the image channle count)
    texturePixels = reinterpret_cast<stbi_uc *>(STBI_MALLOC(texureHeight * textureWidth * 4));
    if (!texturePixels) throw std::runtime_error("Unable to allocate stbi copy buffer");
    memcpy(texturePixels, other.texturePixels, texureHeight * textureWidth * 4);
}

Entity::Entity(Entity&& other) noexcept
: texturePixels(std::exchange(other.texturePixels, nullptr)), textureWidth(other.textureWidth), texureChannels(other.texureChannels),
texureHeight(other.texureHeight), vertices(std::move(other.vertices)), indices(std::move(other.indices)), boundingRadius(other.boundingRadius),
hasConstTexture(other.hasConstTexture)
{}

Entity& Entity::operator=(const Entity& other) {
    return *this = Entity(other);
}

Entity& Entity::operator=(Entity&& other) noexcept {
    std::swap(texturePixels, other.texturePixels);
    textureWidth = other.textureWidth;
    texureHeight = other.texureHeight;
    texureChannels = other.texureChannels;
    hasConstTexture = other.hasConstTexture;
    vertices = std::move(other.vertices);
    indices = std::move(other.indices);
    boundingRadius = other.boundingRadius;
    return *this;
}

Entity::~Entity() {
    if (!hasConstTexture && texturePixels) STBI_FREE(texturePixels);
}

// Temporary function to test something 
Entity Entity::translate(glm::vec3 delta) {
    for(auto& vertex : vertices) {
        vertex.pos = vertex.pos + delta;
    }

    return std::move(*this);
}