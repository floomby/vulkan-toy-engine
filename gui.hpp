#pragma once

#include "utilities.hpp"

#include <boost/lockfree/spsc_queue.hpp>
#include <chrono>
#include <mutex>
#include <thread>
#include <queue>

// I am not sure about using yaml for the gui layout stuff (xml might be better because I could specify a schema and validate it easily)
#include <yaml-cpp/yaml.h>

// This is by no mean a high performance gui, when something is out of date it rebuilds all the vertex data from the tree
// I just want to get it working though for right now (avoid that premature optimization)

struct GuiVertex {
    glm::vec3 pos;
    glm::vec4 color;
    glm::vec2 texCoord;
    glm::uint8 texIndex;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(GuiVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(GuiVertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(GuiVertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(GuiVertex, texCoord);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 4;
        attributeDescriptions[3].format = VK_FORMAT_R8_UINT;
        attributeDescriptions[3].offset = offsetof(GuiVertex, texIndex);

        return attributeDescriptions;
    }
};

#include <ft2build.h>
#include FT_FREETYPE_H

// bleh, this foward declaration, everything is bleeding together
class Engine;

class GuiTexture {
public:
    GuiTexture(Engine *context, void *pixels, int width, int height, int channels, int stride, VkFormat format);
    GuiTexture(const GuiTexture&);
    GuiTexture& operator=(const GuiTexture&);
    GuiTexture(GuiTexture&& other) noexcept;
    GuiTexture& operator=(GuiTexture&& other) noexcept;
    ~GuiTexture();

    ResourceID resourceID();
    bool operator==(const GuiTexture& other) const;

    VkSampler textureSampler;
    VkImageView textureImageView;

    void syncTexturesToGPU(const std::vector<GuiTexture *>& textures);
    static GuiTexture *defaultTexture();
private:
    VkImage textureImage;
    VmaAllocation textureAllocation;
    Engine *context;
};

namespace GuiTextures {
    GuiTexture makeGuiTexture(const char *str);
};

// important that this is non-movable
class GuiComponent {
public:
    GuiComponent() = delete;
    GuiComponent(bool layoutOnly, uint32_t color, std::pair<float, float> c0, std::pair<float, float> c1, int layer);
    GuiComponent(bool layoutOnly, uint32_t color, std::pair<float, float> c0, std::pair<float, float> c1, int layer, GuiTexture texture);
    ~GuiComponent();
    GuiComponent(const GuiComponent& other) = delete;
    GuiComponent(GuiComponent&& other) noexcept = delete;
    GuiComponent& operator=(const GuiComponent& other) = delete;
    GuiComponent& operator=(GuiComponent&& other) noexcept = delete;

    std::vector<GuiComponent *> children;
    GuiComponent *parent;
    // size_t vertexCount();
    // void appendVerticies(std::back_insert_iterator<std::vector<GuiVertex>> vertexIterator);
    // void signalDirty(); // ???? How did I think I was going to use this (I guess components can change their own state or something)

    std::vector<GuiVertex> vertices;

    // indices to parent
    void addComponent(std::queue<int>& childIdices, GuiComponent *component);
    void addComponent(std::queue<int>& childIdices, GuiComponent *component, GuiComponent *parent);
    // indices to child
    void removeComponent(std::queue<int>& childIndices);

    std::vector<GuiTexture *>& mapTextures(std::vector<GuiTexture *>& acm, int& idx);
    GuiTexture texture;

    void buildVertexBuffer(std::vector<GuiVertex>& acm);
private:
    bool layoutOnly;

    void mapTextures(std::vector<GuiTexture *>& acm, std::map<ResourceID, int>& resources, int& idx);
    // I guess just one texture per component?
    void setTextureIndex(int textureIndex);
};

class GuiLabel : public GuiComponent {
public:
    GuiLabel(const char *str, uint32_t textColor, uint32_t backgroundColor, std::pair<float, float> c0, std::pair<float, float> c1, int layer);
    std::string message;
};

// for the quickly updating stuff in the gui (right now just the drag box) 
// Does this even make sense or is it just needlessly complicating things?
struct GuiPushConstant {
    glm::vec2 dragBox[2];
};

class GuiCommandData {
public:
    std::queue<int> childIndices;
    GuiComponent *component;
};

class Gui {
public:
    Gui(float *mouseNormX, float *mouseNormY);
    ~Gui();

    // These should be automatically deleted, but I want to be sure
    Gui(const Gui& other) = delete;
    Gui(Gui&& other) noexcept = delete;
    Gui& operator=(const Gui& other) = delete;
    Gui& operator=(Gui&& other) noexcept = delete;

    size_t updateBuffer(void *buffer, size_t max);
    // Lets not reinvent the wheel here even if stl is sometimes slow

    // set this to true to indicate the need to update the buffers in vram
    bool rebuilt;

    // Takes normalized device coordinates
    // I think these are in the wrong place
    // NOTE: layerZOffset needs to matches a constant in hud.vert
    static constexpr float layerZOffset = 0.001f;
    static std::vector<GuiVertex> rectangle(std::pair<float, float> c0, std::pair<float, float> c1, glm::vec4 color, int layer);
    
    void setDragBox(std::pair<float, float> c0, std::pair<float, float> c1);

    // Gui is going to be a seperate thread
    GuiPushConstant *pushConstant();
    // void lockPushConstant();
    // void unlockPushConstant();

    enum GuiAction {
        GUI_ADD,
        GUI_REMOVE,
        GUI_TERMINATE
    };

    // messages passed to the gui
    struct GuiCommand {
        GuiAction action;
        GuiCommandData *data;
    };

    enum GuiSignal {
        GUI_SOMETHING,
    };

    // messages passed from the gui
    struct GuiMessage {
        GuiSignal signal;
        // GuiMessageData *data;
    };

    void submitCommand(GuiCommand command);
private:
    std::thread guiThread;
    boost::lockfree::spsc_queue<GuiCommand, boost::lockfree::capacity<1024>> guiCommands;
    boost::lockfree::spsc_queue<GuiMessage, boost::lockfree::capacity<1024>> guiMessages;

    // std::mutex constantMutex;
    GuiPushConstant _pushConstant;
    std::mutex dataMutex;
    std::vector<GuiVertex> vertices;
    std::vector<GuiTexture *> textures;
    std::vector<GuiVertex> dragBox;

    void rebuildBuffer();

    void pollChanges();

    static constexpr auto pollInterval = std::chrono::milliseconds(5);

    GuiComponent *root;
};

class Panel {
public:
    Panel(const char *filename);

private:
    YAML::Node root;
};