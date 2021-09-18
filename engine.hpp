#pragma once

#ifdef _glfw3_h_
#error "Please include engine before/instead of GLFW headers"
#endif

// TODO Check that glm is not incleded already (it uses once though so figure out how to do this the right way)

#include "utilities.hpp"
#include "instance.hpp"

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
    int maxFramesInFlight;
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

    struct Cammera {
        const float minZoom = 1.0f, maxZoom = 10.0f;
        const float gimbleStop = 0.1f;
        glm::vec3 position;
        glm::vec3 target;
    } cammera;

    EngineSettings engineSettings;

    float maxSamplerAnisotropy;
    size_t minUniformBufferOffsetAlignment;

    void initWidow();
    void initVulkan();
    
    GLFWwindow *window;
    bool framebufferResized;
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    enum MouseAction {
        MOUSE_NONE,
        MOUSE_PANNING,
        MOUSE_ROTATING
    } mouseAction;

    struct {
        float x, y;
    } lastMousePosition;

    bool mouseButtonsPressed[8];
    struct MouseEvent {
        int action, button, mods;
        float x, y;
    };
    boost::lockfree::spsc_queue<MouseEvent, boost::lockfree::capacity<1024>> mouseInput;
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    float scrollAmount;
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

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();

    std::vector<VkImageView> swapChainImageViews;
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    void createImageViews();

    VkRenderPass renderPass;
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    void createRenderPass();

    VkDescriptorSetLayout descriptorSetLayout;
    void createDescriptorSetLayout();

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createGraphicsPipeline();

    VkCommandPool commandPool;
    VkCommandPool transferCommandPool;
    void createCommandPools();

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLevels = 1);
    VkCommandBuffer beginSingleTimeCommands();
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool pool);
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool pool, VkQueue queue);
    bool hasStencilComponent(VkFormat format);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);
    void createDepthResources();

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

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VmaAllocation> uniformBufferAllocations;
    size_t uniformSkip;
    void allocateUniformBuffers(size_t instanceCount);
    void reallocateUniformBuffers(size_t instanceCount);
    void destroyUniformBuffers();

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    void createDescriptors(const std::vector<InternalTexture>& textures);

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> transferCommandBuffers;
    void allocateCommandBuffers();

    void recordCommandBuffer(int index);

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    void initSynchronization();

    void recreateSwapChain();

    int currentFrame;
    int commandBufferIndex;
    void drawFrame();

    void handleInput();

    void updateScene(uint32_t currentImage);

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

    void addInstance(int entityIndex, glm::mat4 transformationMatrix);

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