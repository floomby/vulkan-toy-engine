#pragma once

#include "utilities.hpp"

// For right now lets just draw triangles and get the rest right
struct GuiVertex {
    glm::vec3 pos;
    glm::vec4 color;

    // GuiVertex(GuiVertex& mE) = default;
    // GuiVertex& operator=(GuiVertex& mE) = default;
    // GuiVertex(GuiVertex&& mE) = default;
    // GuiVertex& operator=(GuiVertex&& mE) = default;

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

// for the quickly updating stuff in the gui (right now just the drag box)
struct GuiPushConstant {
    glm::vec2 dragBox[2];
};

class GuiComponent {
public:
    std::vector<GuiComponent *> children;
    size_t vertexsCount();
    void appendVerticies(std::back_insert_iterator<std::vector<GuiVertex>> vertexIterator);
};

class Gui {
public:
    Gui();
    void updateBuffer(void *buffer, size_t max);
    // Lets not reinvent the wheel here even if stl is sometimes slow

    bool dirty;
    void rebuildData();

    size_t verticiesCount();
    // Takes normalized device coordinates
    void addRectangle(std::pair<float, float> c0, std::pair<float, float> c1, glm::vec4 color, int layer);
    std::vector<GuiVertex> rectangle(std::pair<float, float> c0, std::pair<float, float> c1, glm::vec4 color, int layer);
    void setDragBox(std::pair<float, float> c0, std::pair<float, float> c1);

    // Gui is going to be a seperate thread
    GuiPushConstant *pushConstant();
    void lockPushConstant();
    void unlockPushConstant();
private:
    GuiPushConstant _pushConstant;
    std::vector<GuiVertex> data;
    std::vector<GuiVertex> dragBox;

    // This matches with a value in the hud vertex shader
    static constexpr float layerZOffset = 0.001f;
};






