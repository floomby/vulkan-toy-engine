#pragma once

#include "utilities.hpp"

// For right now lets just draw triangles and get the rest right
struct GuiVertex {
    glm::vec3 pos;
    glm::vec4 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(GuiVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(GuiVertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(GuiVertex, color);

        return attributeDescriptions;
    }
};

class Gui {
public:
    Gui();
    void updateUniformBuffer(void *buffer, size_t max);
    // Lets not reinvent the wheel here even if stl is sometimes slow
    std::vector<GuiVertex> data = {
        {{ -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
        {{ -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
        {{ 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
        {{ 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
        {{ -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
        {{ 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }}
    };
    // this member is whatever (like stupid)
    size_t verticiesCount();
    // Takes normalized device coordinates
    void addRectangle(std::pair<float, float> c0, std::pair<float, float> c1, glm::vec4 color, int layer);
private:
    // wtf c++? (Like why)
    // A defaulted copy assignment operator for class T is defined as deleted if any of the following is true:
    //  * T has a non-static data member of non-class type (or array thereof) that is const
    static constexpr float layerZOffset = 0.001f;
};






