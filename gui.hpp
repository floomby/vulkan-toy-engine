#pragma once

#include "shaders/render_modes.h"
#include "utilities.hpp"
#include "lua_wrapper.hpp"

#include <boost/lockfree/spsc_queue.hpp>
#include <chrono>
#include <map>
#include <mutex>
#include <thread>
#include <queue>

// I am not sure about using yaml for the gui layout stuff (xml might be better because I could specify a schema and validate it easily)
// #include <yaml-cpp/yaml.h>

// This is by no mean a high performance gui, when something is out of date it rebuilds all the vertex data from the tree
// I just want to get it working though for right now (avoid that premature optimization)

struct GuiVertex {
    glm::vec3 pos;
    glm::vec4 color;
    glm::vec4 secondaryColor;
    glm::vec2 texCoord;
    glm::uint32_t texIndex;
    glm::uint32_t guiID;
    glm::uint32_t renderMode;
    glm::uint32_t flags;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(GuiVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    // I feel like all this duplicated data in the vertex buffer is wastefull where I could have a dynamic ubo, but this makes the
    // drawing have extra steps as well as needing more an additional descriptor and buffer for the ubos as well as the added task
    // of making the reallocations of that buffer just like the dynamic ubos for instances.
    // I am not sure when using a dynamic ubo buffer being better is. I may have crossed into that territory already.
    // Although the non interpolated is actually only 96 bytes per component
    // Alternatively I could just use a static ubo and do a lookup with an id I put here (this somehow seems antithetical to what you are
    // meant to do)
    static std::array<VkVertexInputAttributeDescription, 7> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 7> attributeDescriptions {};

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
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(GuiVertex, secondaryColor);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(GuiVertex, texCoord);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32_UINT;
        attributeDescriptions[4].offset = offsetof(GuiVertex, texIndex);

        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].format = VK_FORMAT_R32_UINT;
        attributeDescriptions[5].offset = offsetof(GuiVertex, guiID);

        attributeDescriptions[6].binding = 0;
        attributeDescriptions[6].location = 6;
        attributeDescriptions[6].format = VK_FORMAT_R32_UINT;
        attributeDescriptions[6].offset = offsetof(GuiVertex, renderMode);

        attributeDescriptions[7].binding = 0;
        attributeDescriptions[7].location = 7;
        attributeDescriptions[7].format = VK_FORMAT_R32_UINT;
        attributeDescriptions[7].offset = offsetof(GuiVertex, flags);

        return attributeDescriptions;
    }
};

#include <ft2build.h>
#include FT_FREETYPE_H

// bleh, this foward declaration, everything is bleeding together
class Engine;

class GuiTexture {
public:
    GuiTexture(Engine *context, void *pixels, int width, int height, int channels, int strideBytes, VkFormat format, VkFilter = VK_FILTER_LINEAR);
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
    float widenessRatio;
private:
    VkImage textureImage;
    VmaAllocation textureAllocation;
    Engine *context;
};

namespace GuiTextures {
    GuiTexture makeGuiTexture(const char *str);
}

class Gui;

#include <iostream>

// important that this is non-movable
class GuiComponent {
public:
    GuiComponent() = delete;
    GuiComponent(Gui *context, bool layoutOnly, uint32_t color, std::pair<float, float> c0,
        std::pair<float, float> c1, int layer, std::map<std::string, int> luaHandlers, uint32_t renderMode = RMODE_FLAT);
    GuiComponent(Gui *context, bool layoutOnly, uint32_t color, std::pair<float, float> c0,
         std::pair<float, float> c1, int layer, std::vector<GuiTexture> textures, std::map<std::string, int> luaHandlers, uint32_t renderMode = RMODE_FLAT);
    GuiComponent(Gui *context, bool layoutOnly, uint32_t color, uint32_t secondaryColor,
        std::pair<float, float> c0, std::pair<float, float> c1, int layer, std::vector<GuiTexture> textures, std::map<std::string, int> luaHandlers, uint32_t renderMode = RMODE_FLAT);
    GuiComponent(Gui *context, bool layoutOnly, uint32_t color, uint32_t secondaryColor,
        std::pair<float, float> tl, float height, int layer, std::vector<GuiTexture> textures, std::map<std::string, int> luaHandlers, uint32_t renderMode = RMODE_FLAT);
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
    void addComponent(std::queue<uint> childIdices, GuiComponent *component);
    void addComponent(std::queue<uint>& childIdices, GuiComponent *component, GuiComponent *parent);
    // indices to child
    void removeComponent(std::queue<uint>& childIndices);

    std::vector<GuiTexture *>& mapTextures(std::vector<GuiTexture *>& acm, int& idx);
    std::vector<GuiTexture> textures;
    std::vector<uint> textureIndexMap;

    void buildVertexBuffer(std::vector<GuiVertex>& acm, std::map<uint32_t, uint>& indexMap, uint& index);
    inline bool containsNDCPoint(float x, float y)
    { return x > vertices[0].pos.x && y > vertices[0].pos.y && x < vertices[1].pos.x && y < vertices[1].pos.y; }
    uint32_t allContaining(std::vector<GuiComponent *>& acm, float x, float y);
    uint32_t allContaining(std::vector<GuiComponent *>& acm, float x, float y, std::pair<int, int>& idMaxLayer);
    int layer;
    uint32_t renderMode;

    virtual void click(float x, float y);
    uint32_t id, activeTexture = 0;

    GuiComponent *getComponent(std::queue<uint> childIdices);
protected:
    bool dynamicNDC = false;
    virtual void resizeVertices();
    Gui *context;
    std::map<std::string, int> luaHandlers;
private:
    bool layoutOnly;

    void mapTextures(std::vector<GuiTexture *>& acm, std::map<ResourceID, int>& resources, int& idx);
    // I guess just one texture per component?
    void setTextureIndex(int textureIndex);
    GuiComponent *getComponent_i(std::queue<uint>& childIdices);
};

class GuiLabel : public GuiComponent {
public:
    GuiLabel(Gui *context, const char *str, uint32_t textColor, uint32_t backgroundColor, std::pair<float, float> c0, std::pair<float, float> c1, int layer, std::map<std::string, int> luaHandlers);
    GuiLabel(Gui *context, const char *str, uint32_t textColor, uint32_t backgroundColor, std::pair<float, float> tl, float height, int layer, std::map<std::string, int> luaHandlers);
    std::string message;

    // virtual void click(float x, float y);
private:
    virtual void resizeVertices();
};

class GuiSprite : public GuiComponent {
public:
};

// for the quickly updating stuff in the gui (right now just the drag box) 
// Does this even make sense or is it just needlessly complicating things?
struct GuiPushConstant {
    glm::vec2 dragBox[2];
    glm::uint32_t guiID;
};

class GuiCommandData {
public:
    std::queue<uint> childIndices;
    GuiComponent *component;
    struct {
        union {
            float asFloat;
            int asInt;
        } x;
        union {
            float asFloat;
            int asInt;
        } y;
    } position;
    struct {
        union {
            float asFloat;
            int asInt;
        } width;
        union {
            float asFloat;
            int asInt;
        } height;
    } size;
    // the gui object already knows this via the push constants, but that technically creates a synchronization error even if it is maybe just a frame off
    uint32_t id;
    uint action;
    uint flags;
    std::string str;
};

class Gui {
public:
    Gui(float *mouseNormX, float *mouseNormY, int screenHeight, int screenWidth, Engine *context);
    ~Gui();

    // These should be automatically deleted, but I want to be sure
    Gui(const Gui& other) = delete;
    Gui(Gui&& other) noexcept = delete;
    Gui& operator=(const Gui& other) = delete;
    Gui& operator=(Gui&& other) noexcept = delete;

    std::tuple<size_t, std::map<uint32_t, uint>, VkBuffer> updateBuffer();

    // set this to true to indicate the need to update the buffers in vram
    std::atomic<bool> rebuilt;

    // Takes normalized device coordinates
    // I think these are in the wrong place
    static std::vector<GuiVertex> rectangle(std::pair<float, float> tl, std::pair<float, float> br, glm::vec4 color,
        int layer, uint32_t id, uint32_t renderMode);
    static std::vector<GuiVertex> rectangle(std::pair<float, float> tl, std::pair<float, float> br, glm::vec4 color,
        glm::vec4 secondaryColor, int layer, uint32_t id, uint32_t renderMode);
    std::vector<GuiVertex> rectangle(std::pair<float, float> tl, float height, float widenessRatio, glm::vec4 color,
        glm::vec4 secondaryColor, int layer, uint32_t id, uint32_t renderMode);
    
    void setDragBox(std::pair<float, float> c0, std::pair<float, float> c1);
    void setCursorID(uint32_t id);

    GuiPushConstant *pushConstant();
    // I see no reason to need to lock the constants since the worst we can do is create a slight obliqueness which last 1 frame
    // in the drag box and that is even pretty unlikely
    // void lockPushConstant();
    // void unlockPushConstant();

    enum GuiAction {
        GUI_ADD,
        GUI_REMOVE,
        GUI_RESIZE,
        GUI_CLICK,
        GUI_LOAD,
        GUI_TERMINATE
    };

    // messages passed to the gui
    struct GuiCommand {
        GuiAction action;
        GuiCommandData *data;
    };

    enum GuiSignal {
        GUI_CLICKED,
        // GUI_TOGGLE_TEXTURE,
    };

    struct GuiMessageData {
        uint32_t id;
        int texture;
    };

    // messages passed from the gui
    struct GuiMessage {
        GuiSignal signal;
        GuiMessageData *data;
    };

    void submitCommand(GuiCommand command);
    boost::lockfree::spsc_queue<GuiMessage, boost::lockfree::capacity<1024>> guiMessages;

    static const int dummyCompomentCount = 2;
    static const int dummyVertexCount = dummyCompomentCount * 6;
    int width, height;
    int idCounter = 0;

    uint32_t idUnderPoint(float x, float y);
    void setTextureIndexInBuffer(GuiVertex *buffer, uint index, int textureIndex);
    std::map<uint32_t, GuiComponent *> idLookup;
    void rebuildBuffer();

    GuiComponent *fromFile(std::string name, int baseLayer);
    LuaWrapper lua;
private:
    std::thread guiThread;
    boost::lockfree::spsc_queue<GuiCommand, boost::lockfree::capacity<1024>> guiCommands;

    // std::mutex constantMutex;
    GuiPushConstant _pushConstant;
    std::mutex dataMutex;
    std::map<uint32_t, uint> idToBuffer;
    std::vector<GuiVertex> vertices;
    std::vector<GuiTexture *> textures;
    std::vector<GuiVertex> dragBox;

    void pollChanges();

    static constexpr auto pollInterval = std::chrono::milliseconds(5);

    GuiComponent *root;

    GuiComponent *fromLayout(GuiLayoutNode *tree, int baseLayer);

    int whichBuffer;
    std::array<VmaAllocation, 2> gpuAllocations;
    std::array<VkBuffer, 2> gpuBuffers;
    std::array<size_t, 2> gpuSizes;
    std::array<size_t, 2> usedSizes;

    void createBuffers(size_t initalSize);
    void reallocateBuffer(int index, size_t newSize);
    void destroyBuffer(int index);

    Engine *context;
};

class Panel {
public:
    Panel(const char *filename);

private:
    // YAML::Node root;
};