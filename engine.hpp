#pragma once

#ifdef _glfw3_h_
#error "Please include engine before/instead of GLFW headers"
#endif

#include "utilities.hpp"
#include "instance.hpp"
#include "gui.hpp"
#include "state.hpp"

// #include <map>
#include <boost/lockfree/spsc_queue.hpp>
#include <set>

#define VMA_DEBUG_LOG(format, ...) do { \
    printf(format, ##__VA_ARGS__); \
    printf("\n"); \
} while(false)

class Scene;

struct EngineSettings {
    bool useConcurrentTransferQueue;
    uint height, width;
    std::vector<const char *> validationLayers;
    std::vector<VkValidationFeatureEnableEXT> validationExtentions;
    // int maxFramesInFlight;
    const char *applicationName;
    bool verbose;
    bool extremelyVerbose;
    size_t maxTextures;
    size_t maxHudTextures;
};

class Engine : Utilities {
    // TODO add stuff to the engine class to be able to remove this friend declaration
    friend class InternalTexture;
    friend class CubeMap;
    // The big thing about these is they dont have mipmap levels
    friend class GuiTexture;
    friend class Scene;
public:
    Engine(EngineSettings engineSettings);
    void init();
    void runCurrentScene();

    ~Engine();
    Engine(const Engine& other) = delete;
    Engine(Engine&& other) noexcept = delete;
    Engine& operator=(const Engine& other) = delete;
    Engine& operator=(Engine&& other) noexcept = delete;

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
    } pushConstants;

    ViewProjPosNearFar lightingData;

    bool guiOutOfDate = false;
    Gui *gui = nullptr;

    struct Cammera {
        const float minZoom2 = 1.0, maxZoom2 = 400.0;
        const float gimbleStop = 0.1;
        const float minClip = 0.1, maxClip = 40.0;
        const float renderAsIcon2 = 64.0;
        glm::vec3 position;
        glm::vec3 target;
        glm::vec3 pointing, strafing, fowarding, heading;
        uint32_t when;
    } cammera;
    void stateObserver(ObservableState& state);

    EngineSettings engineSettings;

    // Maybe shove these all in a struct to help organize them
    float maxSamplerAnisotropy;
    size_t minUniformBufferOffsetAlignment;
    std::array<float, 2> lineWidthRange;
    float lineWidthGranularity;

    // Some device querying stuff
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    VkBool32 formatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling);

    void initWidow();
    void initVulkan();

    GLFWwindow *window;
    bool framebufferResized;
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    struct {
        int width, height;
    } framebufferSize;

    // Mouse ui state stuff
    enum MouseAction {
        MOUSE_NONE,
        MOUSE_PANNING,
        MOUSE_ROTATING,
        MOUSE_DRAGGING
    } mouseAction;

    glm::vec3 dragStartRay;
    std::pair<float, float> dragStartDevice;

    struct {
        float x, y;
        float normedX, normedY;
    } lastMousePosition;

    bool mouseButtonsPressed[8];
    struct MouseEvent {
        int action, button, mods;
        float x, y;
        glm::vec3 ray;
    };
    boost::lockfree::spsc_queue<MouseEvent, boost::lockfree::capacity<1024>> mouseInput;
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    float scrollAmount;

    std::vector<GLFWcursor *> cursors;
    void createCursors();
    void destroyCursors();
    const std::array<std::tuple<const char *, int, int>, 2> cursorData = {
        std::make_tuple("cursors/default.png", 1, 1),
        std::make_tuple("cursors/select.png", 1, 1)
    };
    enum CursorIndex {
        CURSOR_DEFAULT = 0,
        CURSOR_SELECT,
    };

    inline std::pair<float, float> normedDevice(float x, float y);
    glm::vec3 raycast(float x, float y, glm::mat4 inverseProjection, glm::mat4 inverseView);
    glm::vec3 raycastDevice(float normedDeviceX, float normedDeviceY, glm::mat4 inverseProjection, glm::mat4 inverseView);
    glm::vec3 raycastDevice(std::pair<float, float> normedDevice, glm::mat4 inverseProjection, glm::mat4 inverseView);

    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    bool keysPressed[349];
    struct KeyEvent {
        int action, key, mods;
    };
    boost::lockfree::spsc_queue<KeyEvent, boost::lockfree::capacity<1024>> keyboardInput;
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

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

    std::set<std::string> getSupportedExtensions();

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices physicalDeviceIndices;
    static QueueFamilyIndices findQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface, bool useConcurrentTransferQueue);
    bool deviceSupportsExtensions(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    bool deviceSupportsSwapChain(VkPhysicalDevice device);
    void pickPhysicalDevice();

    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;
    VmaAllocator memoryAllocator;
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
    std::vector<VkImage> iconSubpassImages;
    std::vector<VmaAllocation> iconSubpassImageAllocations;
    std::vector<VkImageView> iconSubpassImageViews;

    std::vector<int> zSortedIcons;
    
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
    } mainPass;

    VkDescriptorSetLayout hudDescriptorLayout;
    void createDescriptorSetLayout();

    VkPipelineLayout pipelineLayout;
    // VkPipelineLayout iconPipelineLayout;
    VkPipelineLayout hudPipelineLayout;
    std::vector<VkPipeline> graphicsPipelines;
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createGraphicsPipelines();

    VkCommandPool commandPool;
    VkCommandPool transferCommandPool;
    // Saddly I believe this needs to be on the graphics queue since it is messing with textures, even though it is mostly moving stuff around
    VkCommandPool guiCommandPool;
    void createCommandPools();

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLevels = 1);
    VkCommandBuffer beginSingleTimeCommands();
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool pool);
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool pool, VkQueue queue);
    bool hasStencilComponent(VkFormat format);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);

    std::vector<VkImage> depthImages;
    std::vector<VmaAllocation> depthImageAllocations;
    std::vector<VkImageView> depthImageViews;
    void createDepthResources();
    void destroyDepthResources();

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
    VkBuffer hudBuffer;
    VmaAllocation hudAllocation;
    void createHudBuffers();

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VmaAllocation> uniformBufferAllocations;
    std::vector<VkBuffer> lightingBuffers;
    std::vector<VmaAllocation> lightingBufferAllocations;
    size_t uniformSkip;
    void allocateUniformBuffers(size_t instanceCount);
    void reallocateUniformBuffers(size_t instanceCount);
    void destroyUniformBuffers();
    void updateLightingDescriptors(int index, const ViewProjPosNearFar& data);

    VkDescriptorPool hudDescriptorPool;
    std::vector<VkDescriptorSet> hudDescriptorSets;
    std::vector<bool> descriptorDirty; // I think I am not actually using this
    // I do not like the way I am doing this
    void createDescriptors(const std::vector<InternalTexture>& textures, const std::vector<GuiTexture>& hudTextures);
    void writeHudDescriptors();
    void rewriteHudDescriptors(const std::vector<GuiTexture *>& hudTextures);

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> transferCommandBuffers;
    void allocateCommandBuffers();

    void recordCommandBuffer(const VkCommandBuffer& buffer, const VkFramebuffer& framebuffer, const VkDescriptorSet& descriptorSet, 
        const VkDescriptorSet& hudDescriptorSet, const VkBuffer& hudBuffer, int index);

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

    int currentFrame;
    void drawFrame();

    void handleInput();

    std::array<Vertex, 4> iconPrimative;
    struct ExtraPrimatives {
        int vertexCount = 4;
        int indexCount = 6;
        int vertexStart, indexStart; 
    } extraPrimatives;
    std::vector<bool> lightingDirty;
    void updateScene(int index);

    Scene *currentScene;
    void loadDefaultScene();

    void cleanupSwapChain();
    void cleanup();

    // I think we dont need to worry about this for a single run of the application, only if we want to reload them when we restart the appliction
    // which could be nice in the future
    // VkPipelineCache pipelineCache;

    // shadow render pass stuff
    struct ShadowPushConstansts {
        glm::mat4 projection;
        glm::mat4 view;
        glm::vec3 lightPos;
    };

    const char *shadowMapFile = "shadow_map.png";

    struct {
        const uint32_t width = 2048, height = 2048;
        VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        VkImageView imageView;
        VkImage image;
        VmaAllocation allocation;
        VkSampler sampler;
        VkFramebuffer framebuffer;
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
    void writeShadowBufferToFile(const VkCommandBuffer& buffer, const char *filename);
    void doShadowDebugWrite();
};

class CubeMap {
public:
    // front, back, up, down, right, left
    CubeMap(Engine *context, std::array<const char *, 6> files);
    CubeMap(const CubeMap&);
    CubeMap& operator=(const CubeMap&);
    CubeMap(CubeMap&& other) noexcept;
    CubeMap& operator=(CubeMap&& other) noexcept;
    ~CubeMap();

    VkSampler textureSampler;
    VkImageView textureImageView;
private:
    VkImage textureImage;
    VmaAllocation textureAllocation;
    Engine *context;
};

namespace GuiTextures {
    void initFreetype2(Engine *context);
    int makeTexture(const char *str);
    void setDefaultTexture();
};

struct instanceZSorter {
    instanceZSorter(Scene *context);
    bool operator() (int a, int b);
private:
    Scene *context;
};

class Scene {
public:
    // vertex and index offsets for the model
    Scene(Engine* context, std::vector<std::tuple<const char *, const char *, const char *>>, size_t initalSize, std::array<const char *, 6> skyboxImages);

    // I don't really want to accidentally be copying or moving the scene even though it is now safe to do so
    Scene(const Scene& other) = delete;
    Scene(Scene&& other) noexcept = delete;
    Scene& operator=(const Scene& other) = delete;
    Scene& operator=(Scene&& other) noexcept = delete;

    CubeMap skybox;

    void addInstance(int entityIndex, glm::vec3 position, glm::vec3 heading);

    // Right now these are public so the engine can see them to copy them to vram
    std::vector<Entity> entities;
    std::vector<InternalTexture> textures;
    std::vector<SceneModelInfo> models;

    void makeBuffers();

    std::vector<Utilities::Vertex> vertexBuffer;
    std::vector<uint32_t> indexBuffer;

    // std::vector<Instance> instances;
    ObservableState state;
    std::map<std::string, Panel> panels;
    
    void updateUniforms(void *buffer, size_t uniformSkip);

    instanceZSorter zSorter;
private:
    Engine* context;
};

struct TextureCreationData {
    int height, width, channels;
    unsigned char *pixels;
};

class InternalTexture {
public:
    int mipLevels, width, height, channels;
    
    InternalTexture(Engine *context, TextureCreationData creationData);

    InternalTexture(const InternalTexture&);
    InternalTexture& operator=(const InternalTexture&);
    InternalTexture(InternalTexture&& other) noexcept;
    InternalTexture& operator=(InternalTexture&& other) noexcept;
    ~InternalTexture();

    // I guess we need to make these public
    VkSampler textureSampler;
    VkImageView textureImageView;
private:
    VkImage textureImage;
    VmaAllocation textureAllocation;
    Engine *context;
    static bool compatabilityCheckCompleted;
    static bool supportsLinearBlitting_;
    bool supportsLinearBlitting();

    void generateMipmaps();
};