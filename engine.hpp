#pragma once

#ifdef _glfw3_h_
#error "Please include engine before/instead of GLFW headers"
#endif

// TODO Check that glm is not incleded already (it uses once though so figure out how to do this the right way)

#include "utilities.hpp"
#include "instance.hpp"
#include "gui.hpp"

// #include <map>
#include <boost/lockfree/spsc_queue.hpp>
#include <set>

#define VMA_DEBUG_LOG(format, ...) do { \
    printf(format, ##__VA_ARGS__); \
    printf("\n"); \
} while(false)


#include "libs/vk_mem_alloc.h"

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
};

class Engine : Utilities {
    // TODO add stuff to the engine class to be able to remove this friend declaration
    friend class InternalTexture;
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
    } pushConstants;

    // struct HudPushConstants {
    //     glm::vec2 dragBox[2];
    // } hudConstants;

    Gui gui;

    struct Cammera {
        const float minZoom = 1.0f, maxZoom = 10.0f;
        const float gimbleStop = 0.1f;
        const float minClip = 0.1f, maxClip = 40.0f;
        glm::vec3 position;
        glm::vec3 target;
    } cammera;

    EngineSettings engineSettings;

    // Maybe shove these all in a struct to help organize them
    float maxSamplerAnisotropy;
    size_t minUniformBufferOffsetAlignment;
    std::array<float, 2> lineWidthRange;
    float lineWidthGranularity;

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
    std::vector<VkImageView> swapChainImageViews;
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    void createImageViews();
    void destroyImageViews();

    VkRenderPass renderPass;
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    void createRenderPass();

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSetLayout hudDescriptorLayout;
    void createDescriptorSetLayout();

    VkPipelineLayout pipelineLayout;
    VkPipelineLayout hudPipelineLayout;
    std::vector<VkPipeline> graphicsPipelines;
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createGraphicsPipelines();

    VkCommandPool commandPool;
    VkCommandPool transferCommandPool;
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

    // std::vector<VkBuffer> hudBuffers;
    VkBuffer hudBuffer;
    // std::vector<VmaAllocation> hudAllocations;
    VmaAllocation hudAllocation;
    void createHudBuffers();

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VmaAllocation> uniformBufferAllocations;
    size_t uniformSkip;
    void allocateUniformBuffers(size_t instanceCount);
    void reallocateUniformBuffers(size_t instanceCount);
    void destroyUniformBuffers();

    VkDescriptorPool descriptorPool;
    VkDescriptorPool hudDescriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkDescriptorSet> hudDescriptorSets;
    std::vector<bool> descriptorDirty; // I think I am not actually using this
    // void allocateDescriptors();
    // void updateDescriptor(const std::vector<InternalTexture>& textures, int index);
    void createDescriptors(const std::vector<InternalTexture>& textures);

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> transferCommandBuffers;
    void allocateCommandBuffers();

    void recordCommandBuffer(const VkCommandBuffer& buffer, const VkFramebuffer& framebuffer, const VkDescriptorSet& descriptorSet, 
        const VkDescriptorSet& hudDescriptorSet, const VkBuffer& hudBuffer);

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    void initSynchronization();

    void recreateSwapChain();

    int currentFrame;
    void drawFrame();

    void handleInput();

    void updateScene();

    Scene *currentScene;
    void loadDefaultScene();

    void cleanupSwapChain();
    void cleanup();
};

class Scene {
public:
    // vertex and index offsets for the model
    Scene(Engine* context, std::vector<std::pair<const char *, const char *>>, size_t initalSize);

    ~Scene();
    Scene(const Scene& other) = delete;
    Scene(Scene&& other) noexcept = delete;
    Scene& operator=(const Scene& other) = delete;
    Scene& operator=(Scene&& other) noexcept = delete;

    void addInstance(int entityIndex, glm::vec3 position, glm::vec3 heading);

    // Right now these are public so the engine can see them to copy them to vram
    std::vector<Entity> entities;
    std::vector<InternalTexture> textures;
    std::vector<SceneModelInfo> models;

    void makeBuffers();

    std::vector<Utilities::Vertex> vertexBuffer;
    std::vector<uint32_t> indexBuffer;

    // TODO Make this use something real (ring maybe?)
    size_t currentSize, currentUsed;
    Instance *instances;
    
    void updateUniforms(void *buffer, size_t uniformSkip);
private:
    Engine* context;
};

class InternalTexture {
public:
    int mipLevels, width, height;
    
    InternalTexture(Engine *context, const Entity& entity);

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