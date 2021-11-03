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
}

Entity::Entity(const char *model, const char *texture, const char *icon) {
    hasConstTexture = false;
    if (hasTexture = strlen(texture)) {
        texturePixels = stbi_load(texture, &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
        if (!texturePixels) throw std::runtime_error("Failed to load texture image.");
    }

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model)) throw std::runtime_error(std::string("Unable to load model: ") + warn + err);

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

// such boilerplate (Yeah, I should have done it differently and managed the resources seperately with a shared_ptr or something)
Entity::Entity(const Entity& other) {
    textureWidth = other.textureWidth;
    textureHeight = other.textureHeight;
    textureChannels = other.textureChannels;
    hasConstTexture = other.hasConstTexture;
    iconWidth = other.iconWidth;
    iconHeight = other.iconHeight;
    iconChannels = other.iconChannels;
    hasIcon = other.hasIcon;
    hasTexture = other.hasTexture;
    textureIndex = other.textureIndex;
    iconIndex = other.iconIndex;
    vertices = std::move(other.vertices);
    indices = std::move(other.indices);
    // I am not sure about this code (we may need to check the image channle count)
    if (hasConstTexture) {
        texturePixels = other.texturePixels;
    } else if (hasTexture) {
        texturePixels = reinterpret_cast<stbi_uc *>(STBI_MALLOC(textureHeight * textureWidth * textureChannels));
        if (!texturePixels) throw std::runtime_error("Unable to allocate stbi copy buffer");
        memcpy(texturePixels, other.texturePixels, textureHeight * textureWidth * textureChannels);
    }
    if (hasIcon) {
        iconPixels = reinterpret_cast<stbi_uc *>(STBI_MALLOC(iconHeight * iconWidth * iconChannels));
        if (!iconPixels) throw std::runtime_error("Unable to allocate stbi copy buffer");
        memcpy(iconPixels, other.iconPixels, iconHeight * iconWidth * iconChannels);
    }
}

Entity::Entity(Entity&& other) noexcept
: texturePixels(std::exchange(other.texturePixels, nullptr)), iconPixels(std::exchange(other.iconPixels, nullptr)), textureWidth(other.textureWidth),
textureChannels(other.textureChannels), textureHeight(other.textureHeight), vertices(std::move(other.vertices)), indices(std::move(other.indices)),
boundingRadius(other.boundingRadius), hasConstTexture(other.hasConstTexture), iconChannels(other.iconChannels), iconHeight(other.iconHeight),
iconWidth(other.iconWidth), hasIcon(other.hasIcon), hasTexture(other.hasTexture), textureIndex(other.textureIndex), iconIndex(iconIndex)
{}

Entity& Entity::operator=(const Entity& other) {
    return *this = Entity(other);
}

Entity& Entity::operator=(Entity&& other) noexcept {
    std::swap(texturePixels, other.texturePixels);
    std::swap(iconPixels, other.iconPixels);
    textureWidth = other.textureWidth;
    textureHeight = other.textureHeight;
    textureChannels = other.textureChannels;
    hasConstTexture = other.hasConstTexture;
    iconWidth = other.iconWidth;
    iconHeight = other.iconHeight;
    iconChannels = other.iconChannels;
    hasIcon = other.hasIcon;
    hasTexture = other.hasTexture;
    textureIndex = other.textureIndex;
    iconIndex = other.iconIndex;
    vertices = std::move(other.vertices);
    indices = std::move(other.indices);
    boundingRadius = other.boundingRadius;
    return *this;
}

Entity::~Entity() {
    if (hasTexture && !hasConstTexture && texturePixels) STBI_FREE(texturePixels);
    if (hasIcon && iconPixels) STBI_FREE(iconPixels);
}

// Temporary function to test something 
Entity Entity::translate(glm::vec3 delta) {
    for(auto& vertex : vertices) {
        vertex.pos = vertex.pos + delta;
    }

    return std::move(*this);
}