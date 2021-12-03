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
#include <sstream>
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
            return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
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

    struct ViewProj {
        glm::mat4 view;
        glm::mat4 proj;
    };

    // used for lighting and probably other things in the future
    struct ViewProjPos {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 pos;
    };

    // I think the alignment is wrong for nearFar
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
    static std::vector<char> readFile(const std::string& filename);
protected:
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

    inline ostream& operator<<(ostream& os, const glm::vec4& vec) {
        os << "x:" << vec.x << " y:" << vec.y << " z:" << vec.z << " w:" << vec.w;
        return os;
    }

    inline ostream& operator<<(ostream& os, const glm::vec3& vec) {
        os << "x:" << vec.x << " y:" << vec.y << " z:" << vec.z;
        return os;
    }

    inline ostream& operator<<(ostream& os, const glm::quat& qat) {
        os << "w:" << qat.w << " x:" << qat.x << " y:" << qat.y << " z:" << qat.z;
        return os;
    }

    inline ostringstream& operator<<(ostringstream& os, const glm::vec4& vec) {
        os << "x:" << vec.x << " y:" << vec.y << " z:" << vec.z << " w:" << vec.w;
        return os;
    }

    inline ostringstream& operator<<(ostringstream& os, const glm::vec3& vec) {
        os << "x:" << vec.x << " y:" << vec.y << " z:" << vec.z;
        return os;
    }

    inline ostringstream& operator<<(ostringstream& os, const glm::quat& qat) {
        os << "w:" << qat.w << " x:" << qat.x << " y:" << qat.y << " z:" << qat.z;
        return os;
    }
}

typedef uint32_t InstanceID;
typedef void* ResourceID;

enum class InsertionMode {
    FRONT,
    BACK,
    OVERWRITE,
    NONE
};

template <typename T> int sgn(T x) {
    return (T(0) < x) - (x < T(0));
}

template <typename T> T linmap(T x, T inMin, T inMax, T outMin, T outMax) {
    return ((x - inMin) / (inMax - inMin) + outMin) * (outMax - outMin);
}

// will evaluate x twice
#define sq(x) ((x) * (x))

// I think I will just use the modifiers that glfw has (space is not a modifer for them though)
// #define MOD_SHIFT   (1 << 0)
// #define MOD_CTL     (1 << 1)
// #define MOD_ALT     (1 << 2)
// #define MOD_SPACE   (1 << 3)

inline glm::vec3 clampLength(const glm::vec3& v, float max) {
    auto len = length(v);
    if (len > max) {
        return v * (max / len);
    }
    return v;
}

inline glm::vec3 orthogonal(const glm::vec3& v) {
    float x = fabsf(v.x);
    float y = fabsf(v.y);
    float z = fabsf(v.z);

    glm::vec3 other = x < y ? (x < z ? glm::vec3({ 1.0f, 0.0f, 0.0f }) : glm::vec3({0.0f, 0.0f, 1.0f })) : 
        (y < z ? glm::vec3({0.0f, 1.0f, 0.0f }) : glm::vec3({0.0f, 0.0f, 1.0f }));
    return cross(v, other);
}

inline glm::quat rotationVector(const glm::vec3& u, const glm::vec3& v) {
    float kCosTheta = dot(u, v);
    float k = sqrt(length2(u) * length2(v));

    if (kCosTheta / k == -1) {
        // 180 degree rotation around any orthogonal vector
        return glm::quat(0.0f, normalize(orthogonal(u)));
    }

    return normalize(glm::quat(kCosTheta + k, cross(u, v)));
}

inline glm::vec3 quatToDirection(const glm::quat& q) {
    return {
        2.0f * (q.x * q.z - q.w * q.y),
        2.0f * (q.y * q.z + q.w * q.x),
        1.0f - 2.0f * (q.x * q.x + q.y * q.y)
    };
}

class Textured {
public:
    Textured();
    VkImageView imageView;
    VkSampler sampler;
};

inline bool isSRGB(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_SRGB:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
            return true;
        default:
            return false;
    }
}