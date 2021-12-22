#pragma once

#ifdef _glfw3_h_
#error "Please include engine before/instead of GLFW headers"
#endif

#include "api.hpp"
#include "utilities.hpp"
#include "instance.hpp"
#include "gui.hpp"
#include "state.hpp"
#include "team.hpp"
#include "net.hpp"
#include "base.hpp"

#include <boost/lockfree/spsc_queue.hpp>
#include <codecvt>
#include <locale>
#include <set>

#ifndef DONT_PRINT_MEMORY_LOG
#define VMA_DEBUG_LOG(format, ...) do { \
    printf(format, ##__VA_ARGS__); \
    printf("\n"); \
} while(false)
#endif

class Scene;
class GlyphCache;
class ComputeManager;

struct EngineSettings {
    bool useConcurrentTransferQueue;
    uint height, width;
    std::vector<const char *> validationLayers;
    std::vector<VkValidationFeatureEnableEXT> validationExtentions;
    // int maxFramesInFlight;
    const char *applicationName;
    bool verbose;
    bool extremelyVerbose;
    bool rebuildFontCache;
    Team teamIAm;
};

struct LineHolder {
    std::vector<LineUBO> lines;
    void addCircle(const glm::vec3& center, const glm::vec3& normal, const float radius,
        const uint segmentCount = 20, const glm::vec4& color = glm::vec4({ 1.0f, 1.0f, 1.0f, 1.0f }));
    void addAxes(float length, const glm::vec4& orginColor, const glm::vec4& peripheryColor);
};

template<typename T> class DynUBOSyncer;
template<typename T> class SSBOSyncer;

class Engine;

class TextResource : public Textured {
public:
    TextResource(Engine *context, uint height, uint width);
    ~TextResource();
    void writeDescriptor(VkDescriptorSet set);

    TextResource(const TextResource& other) = delete;
    TextResource(TextResource&& other) noexcept = delete;
    TextResource& operator=(const TextResource& other) = delete;
    TextResource& operator=(TextResource&& other) noexcept = delete;

    VkImage image;
    VmaAllocation allocation;

    static VkSampler sampler_;

    static GuiTexture *toGuiTexture(std::unique_ptr<TextResource>&& textResource);
private:
    float widenessRatio;
    Engine *context;
};

class Engine : public Base {
    // There is probably a better design pattern here
    friend class EntityTexture;
    friend class CubeMap;
    friend class GuiTexture;
    friend class Scene;
    friend class GlyphCache;
    friend class TextResource;
    friend class ComputeManager;
    friend class Api;

    friend class DynUBOSyncer<InstanceUBO>;
    friend class DynUBOSyncer<LineUBO>;

    friend std::tuple<size_t, std::map<uint32_t, uint>, VkBuffer> Gui::updateBuffer();
    friend void ObservableState::syncToAuthoritativeState(AuthoritativeState& state);

public:
    Engine(EngineSettings engineSettings);
    EngineSettings engineSettings;
    void init();
    virtual void runCurrentScene();

    ~Engine();
    Engine(const Engine& other) = delete;
    Engine(Engine&& other) noexcept = delete;
    Engine& operator=(const Engine& other) = delete;
    Engine& operator=(Engine&& other) noexcept = delete;

    VkDevice device;
    VmaAllocator memoryAllocator;
    GlyphCache *glyphCache;

    std::vector<uint32_t> idsSelected;

    virtual void send(const ApiProtocol& data);
    virtual void send(ApiProtocol&& data);
    virtual void quit();

    float fps = 0.0f;

    enum {
        APIL_SELECTION,
        APIL_KEYBINDINGS,
        APIL_CURSOR_INSTANCE,
        APIL_COUNT
    };

    std::array<std::mutex, static_cast<size_t>(APIL_COUNT)> apiLocks;
private:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> transferFamily;

        std::set<uint32_t> families();

        bool isSupported();

        std::vector<const char *> whatIsMissing();

        // for debugging
        std::string info();
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct PushConstants {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 normedPointing;
        glm::uint32_t textureIndex;
        glm::uint32_t renderType;
        alignas(16) glm::vec3 teamColor;
        glm::float32_t blinkTime;
    } pushConstants;

    Utilities::ViewProjPosNearFar lightingData;
    glm::mat4 lightingDataView_1;
    Utilities::ViewProj linePushConstans;

    bool guiOutOfDate = false;
    // Gui *gui = nullptr;

    struct Cammera {
        glm::vec3 position;
        glm::vec3 target;
        glm::vec3 pointing, strafing, fowarding, heading;
        struct {
            glm::mat4 view_1;
            glm::mat4 proj_1;
            glm::mat4 projView;
            glm::mat4 view_1Proj_1;
        } cached;
    } cammera;
    void stateObserver(ObservableState& state);

    std::chrono::time_point<std::chrono::steady_clock> lastTime;
    int serverTicksSinceLastSynchronization;

    // Maybe shove these all in a struct to help organize them
    float maxSamplerAnisotropy;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDeviceProperties& physicalDeviceProperties);
    template<typename T> size_t calculateUniformSkipForUBO() {
        return sizeof(T) / minUniformBufferOffsetAlignment + (sizeof(T) % minUniformBufferOffsetAlignment ? minUniformBufferOffsetAlignment : 0);
    }
    size_t minUniformBufferOffsetAlignment;
    std::array<float, 2> lineWidthRange;
    float lineWidthGranularity;

    // Some device querying stuff
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    VkBool32 formatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling);

    void initWidow();
    void initVulkan();

    void handleConsoleInput();
    std::atomic<bool> done = false;
    std::thread consoleThread;
    LuaWrapper *consoleLua;

    GLFWwindow *window;
    bool framebufferResized;
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    struct {
        int width, height;
    } framebufferSize;

    // Mouse ui state stuff
    bool placingStructure = false;
    enum MouseAction {
        MOUSE_NONE,
        MOUSE_PANNING,
        MOUSE_ROTATING,
        MOUSE_DRAGGING,
        MOUSE_MOVING_Z,
        MOUSE_PLACING_Z
    } mouseAction;
    bool wasZMoving = false, wasZPlacing = false;
    bool drawTooltip = false;
    std::vector<bool> tooltipDirty;
    std::shared_ptr<GuiTexture> tooltipResource, tooltipStillInUse;
    uint64_t tooltipJob = 0;
    void setTooltip(std::string&& str);
    void setTooltip(const std::string& str);

    glm::vec3 dragStartRay;
    std::pair<float, float> dragStartDevice;

    struct {
        float x, y;
        float normedX, normedY;
    } lastMousePosition;

    glm::vec3 movingTo;

    bool mouseButtonsPressed[8];
    struct MouseEvent {
        int action, button, mods;
        float x, y;
        glm::vec3 ray;
    };
    InstanceID mousedOverId = 0;
    bool selectionCanAttack = false, canAttack = false;

    boost::lockfree::spsc_queue<MouseEvent, boost::lockfree::capacity<1024>> mouseInput;
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    float scrollAmount;

    std::vector<GLFWcursor *> cursors;
    void createCursors();
    void destroyCursors();
    const std::array<std::tuple<const char *, int, int>, 3> cursorData = {
        std::make_tuple("cursors/default.png", 1, 1),
        std::make_tuple("cursors/select.png", 1, 1),
        std::make_tuple("cursors/attack.png", 1, 1)
    };
    enum CursorIndex {
        CURSOR_DEFAULT ,
        CURSOR_SELECT,
        CURSOR_ATTACK
    };
    Instance cursorInstance, cursorRender;
    void setCursorInstance(const Instance& instance);
    void setCursorInstance(Instance&& instance);
    void setCursorEntity(const std::string& name);
    void removeCursorEntity();

    inline std::pair<float, float> normedDevice(float x, float y);
    glm::vec3 raycast(float x, float y, glm::mat4 inverseProjection, glm::mat4 inverseView);
    glm::vec3 raycastDevice(float normedDeviceX, float normedDeviceY, glm::mat4 inverseProjection, glm::mat4 inverseView);
    glm::vec3 raycastDevice(std::pair<float, float> normedDevice, glm::mat4 inverseProjection, glm::mat4 inverseView);

    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
    bool keysPressed[349];
    struct KeyEvent {
        int action, key, mods;
    };
    std::set<int> luaKeyBindings;
    boost::lockfree::spsc_queue<KeyEvent, boost::lockfree::capacity<1024>> keyboardInput;
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    bool keyboardCaptured = false;
    static void charCallback(GLFWwindow *window, unsigned int codepoint);

    std::vector<const char *> getUnsupportedLayers();
    void enableValidationLayers();

    std::vector<const char *> getRequiredExtensions();

    VkDebugUtilsMessengerEXT debugMessenger;
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo);
    VkInstance instance;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    void initInstance();

    std::set<std::string> getSupportedInstanceExtensions();

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices physicalDeviceIndices;
    static QueueFamilyIndices findQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface, bool useConcurrentTransferQueue);
    bool deviceSupportsExtensions(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    bool deviceSupportsSwapChain(VkPhysicalDevice device);
    void pickPhysicalDevice();

    VkQueue graphicsQueue;
    VkQueue guiGraphicsQueue;
    VkQueue computeQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;
    void setupLogicalDevice();

    uint32_t concurrentFrames;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();

    std::vector<VkImage> subpassImages;
    std::vector<VmaAllocation> subpassImageAllocations;
    std::vector<VkImageView> subpassImageViews;
    std::vector<VkImage> bgSubpassImages;
    std::vector<VmaAllocation> bgSubpassImageAllocations;
    std::vector<VkImageView> bgSubpassImageViews;

    std::vector<int> zSorted;
    
    std::vector<VkImageView> swapChainImageViews;
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    void createImageViews();
    void destroyImageViews();

    VkRenderPass renderPass;
    void createRenderPass();

    struct {
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;
        VkDescriptorSetLayout lineDescriptorLayout;
        VkDescriptorPool lineDescriptorPool;
        std::vector<VkDescriptorSet> lineDescriptorSets;
    } mainPass;

    VkDescriptorSetLayout hudDescriptorLayout;
    void createDescriptorSetLayout();

    VkShaderModule createShaderModule(const std::vector<char>& code);
    VkPipelineLayout pipelineLayout;
    VkPipelineLayout linePipelineLayout;
    VkPipelineLayout hudPipelineLayout;
    enum {
        GP_BG,
        GP_WORLD,
        GP_LINES,
        GP_HUD,
        GP_COUNT
    };
    std::array<VkPipeline, GP_COUNT> graphicsPipelines;
    void createGraphicsPipelines();
    enum {
        CP_SDF_BLIT,
        CP_COUNT
    };
    std::array<VkPipelineLayout, CP_COUNT> computePipelineLayouts;
    std::array<VkPipeline, CP_COUNT> computePipelines;
    std::array<VkDescriptorSetLayout, CP_COUNT> computeSetLayouts;
    std::array<VkDescriptorPool, CP_COUNT> computePools;
    std::array<VkDescriptorSet, CP_COUNT> computeSets;
    void createComputeResources();
    void destroyComputeResources();

    VkBuffer sdfUBOBuffer;
    VmaAllocation sdfUBOAllocation;
    VkFence sdfBlitFence;

    VkCommandPool commandPool;
    VkCommandPool transferCommandPool;
    VkCommandPool guiCommandPool;
    VkCommandPool computeCommandPool;
    void createCommandPools();

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLevels = 1);
    VkCommandBuffer beginSingleTimeCommands();
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool pool);
    // I could use raii pattern to autofree stuff like the way scoped_lock works (Idk what is best, I don't think to hard about it)
    std::function<void (void)> endSingleTimeCommands(VkCommandBuffer commandBuffer);
    std::function<void (void)> endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool pool, VkQueue queue);
    std::function<void (void)> endSingleTimeCommands(VkCommandBuffer commandBuffer, VkFence fence);
    std::function<void (void)> endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool pool, VkQueue queue, VkFence fence);
    bool hasStencilComponent(VkFormat format);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);

    std::vector<VkImage> depthImages;
    std::vector<VmaAllocation> depthImageAllocations;
    std::vector<VkImageView> depthImageViews;
    std::vector<VkImage> depthGuiImages;
    std::vector<VmaAllocation> depthGuiImageAllocations;
    std::vector<VkImageView> depthGuiImageViews;
    void createDepthResources();
    void destroyDepthResources();
    std::vector<VkImage> colorImages;
    std::vector<VmaAllocation> colorImageAllocations;
    std::vector<VkImageView> colorImageViews;
    void createColorResources();
    void destroyColorResources();

    std::vector<VkFramebuffer> swapChainFramebuffers;
    void createFramebuffers();

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory,
        bool concurrent = false, std::vector<uint32_t> queues = { });
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool pool, VkQueue queue);
    void createModelBuffers();

    size_t hudVertexCount;
    std::map<uint32_t, uint> guiIdToBufferIndex;
    VkBuffer hudBuffer;
    void setTooltipTexture(int index, std::shared_ptr<GuiTexture> texture);
    // void createHudBuffers();

    std::list<LineHolder *> lineObjects;
    LineHolder *cursorLines, *staticLines;

    DynUBOSyncer<InstanceUBO> *uniformSync;
    DynUBOSyncer<LineUBO> *lineSync;
    std::vector<VkBuffer> lightingBuffers;
    std::vector<VmaAllocation> lightingBufferAllocations;
    // size_t uniformSkip;
    void allocateLightingBuffers();
    void destroyLightingBuffers();
    void updateLightingDescriptors(int index, const Utilities::ViewProjPosNearFar& data);

    VkDescriptorPool hudDescriptorPool;
    std::vector<VkDescriptorSet> hudDescriptorSets;
    std::vector<bool> descriptorDirty; // I think I am not actually using this
    // I do not like the way I am doing this
    void createMainDescriptors(const std::vector<EntityTexture>& textures, const std::vector<GuiTexture>& hudTextures);
    void writeHudDescriptors();
    std::vector<size_t> hudDescriptorWriteCount;
    std::vector<bool> hudDescriptorNeedsRewrite;
    void rewriteHudDescriptors(int index);
    void rewriteHudDescriptors(const std::vector<std::shared_ptr<GuiTexture>>& hudTextures);

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> transferCommandBuffers;
    void allocateCommandBuffers();

    int iconModelIndex;
    void recordCommandBuffer(const VkCommandBuffer& buffer, const VkFramebuffer& framebuffer, const VkDescriptorSet& descriptorSet, 
        const VkDescriptorSet& hudDescriptorSet, int index);

    struct {
        bool makeDump;
        bool writePending;
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
    } depthDump;
    void dumpDepthBuffer(const VkCommandBuffer& buffer, int index);
    std::array<int, 256>& logDepthBufferToFile(std::array<int, 256>& depthHistogram, const char *filename);

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    void initSynchronization();

    void recreateSwapChain();

    unsigned long totalFrames = 0;
    int currentFrame;
    void drawFrame();

    void handleInput();

    float updateScene(int index);

    // Scene *currentScene;
    void loadDefaultScene();

    void cleanupSwapChain();
    void cleanup();

    // I read on the nvidia docs you should use one even if not writing them to disc
    VkPipelineCache pipelineCache;

    // shadow render pass stuff
    struct ShadowPushConstansts {
        glm::mat4 projection;
        glm::mat4 view;
        glm::vec3 lightPos;
    };

    const char *shadowMapFile = "shadow_map.png";

    struct {
        bool needed = true;
        const uint32_t width = 4096, height = 4096;
        VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        std::vector<VkImageView> imageViews;
        std::vector<VkImage> images;
        std::vector<VmaAllocation> allocations;
        std::vector<VkSampler> samplers;
        std::vector<VkFramebuffer> framebuffers;
        ShadowPushConstansts constants;
        VkDescriptorSetLayout descriptorLayout;
        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;
        VkBuffer debugBuffer;
        VmaAllocation debugAllocation;
        VmaAllocationInfo debugAllocInfo;
        bool debugging;
        bool makeSnapshot;
        bool debugWritePending;
        std::string filename;
    } shadow;

    void createShadowResources(bool createDebugging);
    void destroyShadowResources();
    void createShadowDescriptorSets();
    void runShadowPass(const VkCommandBuffer& buffer, int index);
    // for debugging
    void writeShadowBufferToFile(const VkCommandBuffer& buffer, const char *filename, int idx);
    void doShadowDebugWrite();

    ComputeManager *manager;

    std::shared_ptr<Networking::Client> client;

    std::array<uint64_t, 50> frameTimes;
    int frameTimeIndex = 0;
};

namespace GuiTextures {
    void initFreetype2(Engine *context);
    int makeTexture(const char *str);
    void setDefaultTexture();
}

// the alignment appears to be correct, but only the first 58 bytes work? (Idk what I was talking about, maybe this was the ssbo sync bug?)
struct GlyphInfoUBO {
    glm::uint32_t pixelOffset;
    glm::uint32_t totalWidth;
    glm::uint32_t lineCount;
    glm::uint32_t lineHeight;
    // I should read the rules about vulkan memomry alignment cause this seems stupid to have to make this 4 times as big
    // as it seems like it should be
    glm::uint32_t writeCounts[ECONF_MAX_LINES_IN_TEXT * 4];
};

struct IndexWidthSSBO {
    glm::uint32_t index;
    glm::uint32_t width;
};

// We won't cache control characters as this makes no sense
class GlyphCache {
public:
    GlyphCache(Engine *context, const std::vector<char32_t>& glyphsWanted, bool rebuildFontCache);
    ~GlyphCache();

    struct GlyphData {
        uint width;
        std::shared_ptr<GuiTexture> onGpu;
        uint descriptorIndex;
    };
    std::map<char32_t, GlyphData> glyphDatum;
    uint bufferWidth;
    uint height, lineHeight;

    void writeDescriptors(VkDescriptorSet& set, uint32_t binding);
    std::pair<uint, uint> writeUBO(GlyphInfoUBO *buffer, const std::string& str, bool cacheWidth = false);

    // I don't know if I want to cache widths or not
    std::map<std::string, uint> cachedWidths;
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    // This is blocking
    std::shared_ptr<GuiTexture> makeGuiTexture(const std::string& str);
private:
    SSBOSyncer<IndexWidthSSBO> *syncer;
    GuiTexture *defaultGlyph;
    static constexpr uint defaultGlyphWidth = 16;
    Engine *context;
};

// I don't know if I want to z sort the lines to do transparency in the lines
// The graphics pipeline is configured with blending enabled and the line coloring
// ubos support transparency
// struct LineZSorter {
//     LineZSorter(Scene *context);
//     bool operator() (const LineUBO& a, const LineUBO& b);
// private:
//     Scene *context;
// };

#include "scene.hpp"

struct TextureCreationData {
    int height, width, channels;
    unsigned char *pixels;
};

class EntityTexture : public Textured {
public:
    int mipLevels, width, height, channels;
    
    EntityTexture(Engine *context, TextureCreationData creationData);

    EntityTexture(const EntityTexture&);
    EntityTexture& operator=(const EntityTexture&);
    EntityTexture(EntityTexture&& other) noexcept;
    EntityTexture& operator=(EntityTexture&& other) noexcept;
    ~EntityTexture();
private:
    VkImage image;
    VmaAllocation allocation;
    Engine *context;
    static bool compatabilityCheckCompleted;
    static bool supportsLinearBlitting_;
    bool supportsLinearBlitting();

    void generateMipmaps();
};

struct DescriptorSyncPoint {
    std::vector<VkDescriptorSet> *descriptorSets;
    uint binding;
};

template<typename T> class DynUBOSyncer {
public:
    DynUBOSyncer(Engine *context, const std::vector<DescriptorSyncPoint>& syncPoints, size_t initialSize = 50)
    : context(context), syncPoints(syncPoints), info({
        NULL, 0, VK_WHOLE_SIZE
    }), writer({
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, NULL,
        0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        nullptr, &info, nullptr
    }) {
        uniformBuffers.resize(context->concurrentFrames);
        uniformBufferAllocations.resize(context->concurrentFrames);
        currentSize.resize(context->concurrentFrames);
        validCount.resize(context->concurrentFrames);
        uniformSkip = context->calculateUniformSkipForUBO<T>();

        info.range = uniformSkip;
        for (int i = 0; i < uniformBuffers.size(); i++) {
            currentSize[i] = initialSize;
            VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = initialSize * uniformSkip;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            VmaAllocationCreateInfo bufferAllocationInfo = {};
            bufferAllocationInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            bufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

            if (vmaCreateBuffer(context->memoryAllocator, &bufferInfo, &bufferAllocationInfo, uniformBuffers.data() + i,
                uniformBufferAllocations.data() + i, nullptr) != VK_SUCCESS)
                throw std::runtime_error("Unable to allocate memory for uniform buffers.");

            info.buffer = uniformBuffers[i];

            for (const auto& syncPoint : syncPoints) {
                writer.dstSet = (*syncPoint.descriptorSets)[i];
                writer.dstBinding = syncPoint.binding;
                vkUpdateDescriptorSets(context->device, 1, &writer, 0, nullptr);
            }
        }
    }
    void sync(int descriptorIndex, size_t count, std::function<T *(size_t idx)> func);
    void sync(int descriptorIndex, size_t count, std::function<T ()> func);
    void sync(int descriptorIndex, const std::vector<T>& items);
    ~DynUBOSyncer() {
        for (int i = 0; i < uniformBuffers.size(); i++)
            vmaDestroyBuffer(context->memoryAllocator, uniformBuffers[i], uniformBufferAllocations[i]);
    }
    void rebindSyncPoints(std::vector<DescriptorSyncPoint> syncPoints) {
        this->syncPoints = syncPoints;
        for (int i = 0; i < context->concurrentFrames; i++) { 
            info.buffer = uniformBuffers[i];
            for (const auto& syncPoint : syncPoints) {
                writer.dstSet = (*syncPoint.descriptorSets)[i];
                writer.dstBinding = syncPoint.binding;
                vkUpdateDescriptorSets(context->device, 1, &writer, 0, nullptr);
            }
        }
    }
    size_t uniformSkip;
    std::vector<size_t> validCount;
private:
    void reallocateBuffer(int idx, size_t newSize) {
        vmaDestroyBuffer(context->memoryAllocator, uniformBuffers[idx], uniformBufferAllocations[idx]);

        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = newSize * uniformSkip;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo bufferAllocationInfo = {};
        bufferAllocationInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        bufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        if (vmaCreateBuffer(context->memoryAllocator, &bufferInfo, &bufferAllocationInfo, uniformBuffers.data() + idx,
            uniformBufferAllocations.data() + idx, nullptr) != VK_SUCCESS)
            throw std::runtime_error("Unable to allocate memory for uniform buffers.");

        currentSize[idx] = newSize;

        info.buffer = uniformBuffers[idx];

        for (const auto& syncPoint : syncPoints) {
            writer.dstSet = (*syncPoint.descriptorSets)[idx];
            writer.dstBinding = syncPoint.binding;
            vkUpdateDescriptorSets(context->device, 1, &writer, 0, nullptr);
        }
    }

    VkWriteDescriptorSet writer;
    VkDescriptorBufferInfo info;
    Engine *context;
    std::vector<DescriptorSyncPoint> syncPoints;
    std::vector<size_t> currentSize;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VmaAllocation> uniformBufferAllocations;
};

template<typename T> class SSBOSyncer {
public:
    SSBOSyncer(Engine *context, const std::vector<std::tuple<VkDescriptorSet, int>>& syncPoints, size_t initialSize = 50)
    : context(context), syncPoints(syncPoints), info({
        NULL, 0, VK_WHOLE_SIZE
    }), writer({
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, NULL,
        0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        nullptr, &info, nullptr
    }) {
        currentSize = initialSize;
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = initialSize * sizeof(T);
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo bufferAllocationInfo = {};
        bufferAllocationInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        bufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        if (vmaCreateBuffer(context->memoryAllocator, &bufferInfo, &bufferAllocationInfo, &buffer, &bufferAllocation, nullptr) != VK_SUCCESS)
            throw std::runtime_error("Unable to allocate memory for ssbo.");

        info.buffer = buffer;

        for (const auto& [set, binding] : syncPoints) {
            writer.dstSet = set;
            writer.dstBinding = binding;
            vkUpdateDescriptorSets(context->device, 1, &writer, 0, nullptr);
        }
    }
    ~SSBOSyncer() {
        vmaDestroyBuffer(context->memoryAllocator, buffer, bufferAllocation);
    }
    void rebindSyncPoints(const std::vector<std::tuple<VkDescriptorSet, int>>& syncPoints) {
        this->syncPoints = syncPoints;
        info.buffer = buffer;
        for (const auto& [set, binding] : syncPoints) {
            writer.dstSet = set;
            writer.dstBinding = binding;
            vkUpdateDescriptorSets(context->device, 1, &writer, 0, nullptr);
        }
    }
    void sync() {
        for (const auto& [set, binding] : syncPoints) {
            writer.dstSet = set;
            writer.dstBinding = binding;
            vkUpdateDescriptorSets(context->device, 1, &writer, 0, nullptr);
        }
    }
    T *ensureSize(T *buf, size_t count);

private:
    static constexpr size_t increment = 50;

    VkWriteDescriptorSet writer;
    VkDescriptorBufferInfo info;
    Engine *context;
    std::vector<std::tuple<VkDescriptorSet, int>> syncPoints;
    size_t currentSize;
    VkBuffer buffer;
    VmaAllocation bufferAllocation;
};

enum class ComputeKind {
    TEXT,
    TERMINATE
};

struct ComputeOp {
    ComputeKind kind;
    void *data;
    uint64_t id;
    bool deleteData = false;
};

class ComputeManager {
public:
    ComputeManager(Engine *context);
    ~ComputeManager();

    uint64_t submitJob(ComputeOp&& op);
    bool getJob(uint64_t id, ComputeOp& op);

private:
    Engine *context;
    std::mutex inLock, outLock;
    std::queue<ComputeOp> inQueue;
    std::list<ComputeOp> outList;
    std::thread computeThread;
    VkFence fence;
    std::function<void(void)> bufferFreer;

    void poll();
};