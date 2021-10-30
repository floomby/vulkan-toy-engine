#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/common.hpp>

#include <ostream>
#include <string>
#include <vector>

#include "libs/vk_mem_alloc.h"

class Utilities {
public:
    struct Vertex {
        glm::vec3 pos;
        // I don't know if this color is worth having here, I can't bind it per instance, only per entity
        // also right now I am not using it
        glm::vec3 color;
        glm::vec2 texCoord;
        glm::vec3 normal;

        bool operator==(const Vertex& other) const {
            return pos == other.pos && color == other.color && texCoord == other.texCoord;
        }

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription {};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions {};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Vertex, normal);

            return attributeDescriptions;
        }
    };

    // used for lighting and probably other things in the future
    struct ViewProjPos {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 pos;
    };

    struct ViewProjPosNearFar {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 pos;
        glm::vec2 nearFar;
    };
    static inline VkFormat channelsToFormat(int channels) {
        switch (channels) {
            case 1:
                return VK_FORMAT_R8_SRGB;
            case 2:
                return VK_FORMAT_R8G8_SRGB;
            case 3:
                return VK_FORMAT_R8G8B8_SRGB;
            case 4:
                return VK_FORMAT_R8G8B8A8_SRGB;
            default:
                throw std::logic_error("Bad channel count (something is programmed incorrectly)");
        }
    }
protected:
    static std::vector<char> readFile(const std::string& filename);
};

namespace std {
    // This hash function is like whatever
    template<> struct hash<Utilities::Vertex> {
        size_t operator()(Utilities::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };

    inline ostream& operator<<(ostream& os, const glm::vec4& vec)
    {
        os << "x:" << vec.x << " y:" << vec.y << " z:" << vec.z << " w:" << vec.w;
        return os;
    }

    inline ostream& operator<<(ostream& os, const glm::vec3& vec)
    {
        os << "x:" << vec.x << " y:" << vec.y << " z:" << vec.z;
        return os;
    }
}

typedef void* ResourceID;

template <typename T> int sgn(T x) {
    return (T(0) < x) - (x < T(0));
}

#define sq(x) ((x) * (x))