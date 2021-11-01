#define VMA_DEBUG_LOG(format, ...) do { \
    printf(format, ##__VA_ARGS__); \
    printf("\n"); \
} while(false)

#define VMA_IMPLEMENTATION
#include "libs/vk_mem_alloc.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libs/stb_image_write.h"
#include "libs/stb_image.h"

#include "engine.hpp"
#include "entity.hpp"

#include <chrono>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <thread>

#define BOOST_STACKTRACE_USE_ADDR2LINE
#include <boost/stacktrace.hpp>

// The memory from the debugging is not cleaned up correctly
// Debuging stuff
// #define DEPTH_DEBUG_IMAGE_USAGE VK_IMAGE_USAGE_TRANSFER_SRC_BIT
#define DEPTH_DEBUG_IMAGE_USAGE 0

#define LINUX_THREAD_DEBUGGING
#ifdef LINUX_THREAD_DEBUGGING
#include <sys/types.h>
#endif

// TODO, do this same thing for vma errors as well
static std::map<int, const char *> vulkanErrors = {
    { 0, "VK_SUCCESS" },
    { 1, "VK_NOT_READY" },
    { 2, "VK_TIMEOUT" },
    { 3, "VK_EVENT_SET" },
    { 4, "VK_EVENT_RESET" },
    { 5, "VK_INCOMPLETE" },
    { 1, "VK_ERROR_OUT_OF_HOST_MEMORY" },
    { 2, "VK_ERROR_OUT_OF_DEVICE_MEMORY" },
    { 3, "VK_ERROR_INITIALIZATION_FAILED" },
    { 4, "VK_ERROR_DEVICE_LOST" },
    { 5, "VK_ERROR_MEMORY_MAP_FAILED" },
    { 6, "VK_ERROR_LAYER_NOT_PRESENT" },
    { 7, "VK_ERROR_EXTENSION_NOT_PRESENT" },
    { 8, "VK_ERROR_FEATURE_NOT_PRESENT" },
    { 9, "VK_ERROR_INCOMPATIBLE_DRIVER" },
    { 10, "VK_ERROR_TOO_MANY_OBJECTS" },
    { 11, "VK_ERROR_FORMAT_NOT_SUPPORTED" },
    { 12, "VK_ERROR_FRAGMENTED_POOL" },
    { 13, "VK_ERROR_UNKNOWN" },
    { 1000069000, "VK_ERROR_OUT_OF_POOL_MEMORY" },
    { 1000072003, "VK_ERROR_INVALID_EXTERNAL_HANDLE" },
    { 1000161000, "VK_ERROR_FRAGMENTATION" },
    { 1000257000, "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS" },
    { 1000000000, "VK_ERROR_SURFACE_LOST_KHR" },
    { 1000000001, "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" },
    { 1000001003, "VK_SUBOPTIMAL_KHR" },
    { 1000001004, "VK_ERROR_OUT_OF_DATE_KHR" },
    { 1000003001, "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" },
    { 1000011001, "VK_ERROR_VALIDATION_FAILED_EXT" },
    { 1000012000, "VK_ERROR_INVALID_SHADER_NV" },
    { 1000158000, "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT" },
    { 1000174001, "VK_ERROR_NOT_PERMITTED_EXT" },
    { 1000255000, "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" },
    { 1000268000, "VK_THREAD_IDLE_KHR" },
    { 1000268001, "VK_THREAD_DONE_KHR" },
    { 1000268002, "VK_OPERATION_DEFERRED_KHR" },
    { 1000268003, "VK_OPERATION_NOT_DEFERRED_KHR" },
    { 1000297000, "VK_PIPELINE_COMPILE_REQUIRED_EXT" },
    { VK_ERROR_OUT_OF_POOL_MEMORY, "VK_ERROR_OUT_OF_POOL_MEMORY_KHR" },
    { VK_ERROR_INVALID_EXTERNAL_HANDLE, "VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR" },
    { VK_ERROR_FRAGMENTATION, "VK_ERROR_FRAGMENTATION_EXT" },
    { VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT" },
    { VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR" },
    { VK_PIPELINE_COMPILE_REQUIRED_EXT, "VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT" }
};

void vulkanErrorGuard_(VkResult result, const char *errorMessage, const char *file, int line) {
    if (result != VK_SUCCESS) {
        std::stringstream message;
        auto errorName = vulkanErrors.find(result);
        if (errorName == vulkanErrors.end()) {
            message << errorMessage << "\n\tUnknow vulkan error: " << std::dec << result << "   " << file << ":" << line << std::endl;
        } else {
            message << errorMessage << "\n\t" << "Vulkan error: " << std::dec << result << "   " << file << ":" << line << std::endl;
        }
        throw std::runtime_error(message.str());
    }
}

#define vulkanErrorGuard(result, message) vulkanErrorGuard_(result, message, __FILE__, __LINE__)

std::vector<char> Utilities::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::stringstream msg;
        msg << "Failed to open file: " << filename;
        throw std::runtime_error(msg.str());
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        else return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) func(instance, debugMessenger, pAllocator);
}

static const std::vector<const char*> requiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    // My card does not support cubic filtering??? (I am in disbelief, but we can do it in the fragment shader anyways)
    // VK_EXT_FILTER_CUBIC_EXTENSION_NAME
    // I really want to use this, but I can't because nvidia drivers don't support it
    // VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME
};

Engine::Engine(EngineSettings engineSettings) {
    this->engineSettings = engineSettings;
    std::cout << "Engine main thread is " << gettid() << std::endl;
}

// This function is badly named since it only gets *intstance* extensions
std::set<std::string> Engine::getSupportedExtensions() {
    uint32_t count;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
    std::set<std::string> results;

    for (auto & extension : extensions) {
        results.insert(extension.extensionName);
    }

    return results;
}

void Engine::initWidow()
{
    // if (engineSettings.extremelyVerbose) {
    //     std::cout << "Supported extensions:" << std::endl;
    //     for(const auto& name : getSupportedExtensions()) {
    //         std::cout << "\t" << name << std::endl;
    //     }
    // }

    // TODO Get the monitor size and scalling from glfw to give to the freetype init function to make fonts a good size legible
    glfwInit();
    GuiTextures::initFreetype2(this);
    GuiTextures::makeTexture("Tep");
    createCursors();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(engineSettings.width, engineSettings.height, "Vulkan Toy", nullptr, nullptr);
    framebufferSize.width = engineSettings.width;
    framebufferSize.height = engineSettings.height;
    glfwSetWindowUserPointer(window, this);

    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    for (int i = 0; i < 8; i++) mouseButtonsPressed[i] = false;
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    scrollAmount = 0.0f;
    glfwSetScrollCallback(window, scrollCallback);
    memset(&keysPressed, 0, sizeof(keysPressed));
    glfwSetKeyCallback(window, keyCallback);
}

void Engine::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto engine = reinterpret_cast<Engine *>(glfwGetWindowUserPointer(window));
    engine->framebufferResized = true;
    engine->framebufferSize.width = width;
    engine->framebufferSize.height = height;
    // std::cout << "Thread resize callback run on thred: " << gettid() << std::endl;
    if (engine->gui) {
        engine->guiOutOfDate = true; // dont draw the gui until after it has been rebuilt
        GuiCommandData *what = new GuiCommandData();
        // what->childIndices.push(0); // Don't actually push anything rn since we have no root node by default
        what->size.height.asInt = height;
        what->size.width.asInt = width;
        engine->gui->submitCommand({ Gui::GUI_RESIZE, what });
    }
}

void Engine::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto engine = reinterpret_cast<Engine *>(glfwGetWindowUserPointer(window));
    engine->mouseButtonsPressed[button] = action == GLFW_PRESS;
    double x, y;
    glfwGetCursorPos(engine->window, &x, &y);
    engine->mouseInput.push({ action, button, mods, (float)x, (float)y });
    // I guess the tids are the same as the engine main thread and we can just do this???
    // Glfw docs have me confoozzled in this reguard (entirely possible I just can't read)
    // std::cout << "Mouse handler tid: " << gettid() << std::endl;
    auto what = new GuiCommandData();
    what->id = engine->gui->pushConstant()->guiID;
    what->position.x.asFloat = engine->lastMousePosition.normedX;
    what->position.y.asFloat = engine->lastMousePosition.normedY;
    engine->gui->submitCommand({ Gui::GUI_CLICK, what });
}

void Engine::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto engine = reinterpret_cast<Engine *>(glfwGetWindowUserPointer(window));
    engine->scrollAmount -= yoffset;
}

void Engine::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto engine = reinterpret_cast<Engine *>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_UNKNOWN) {
        std::cerr << "Unknown key (ignoring)" << std::endl;
        return;
    }
    engine->keyboardInput.push({ action, key, mods });
}

std::vector<const char *> Engine::getUnsupportedLayers() {
    std::vector<const char *> ret;

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // if (engineSettings.extremelyVerbose) {
        std::cout << "Supported layers:" << std::endl;
        for(const auto& layer : availableLayers) {
            std::cout << "\t" << layer.layerName << std::endl;
        }
    // }

    for (const char* layerName: engineSettings.validationLayers) {
        bool foundLayer = false;

        for(const auto& layerProperties : availableLayers) {
            if (!strcmp(layerName, layerProperties.layerName)) {
                foundLayer = true;
                break;
            }
        }
        if (!foundLayer) ret.push_back(layerName);
    }
    return ret;
}

void Engine::enableValidationLayers() {
    if (engineSettings.validationLayers.size()) {
        auto unsupportedLayers = getUnsupportedLayers();
        if (unsupportedLayers.size() > 0) {
            std::stringstream msg;
            msg << "Requested layers (" << engineSettings.validationLayers.size() << "):" << std::endl;
            for (const auto& layerName : engineSettings.validationLayers) msg << '\t' << layerName << std::endl;
            msg << "but the following layers are unsupported (" << unsupportedLayers.size() << "):" << std::endl;
            for (const char* layerName : unsupportedLayers) msg << '\t' << layerName << std::endl;

            throw std::runtime_error(msg.str());
        }
    }
}

std::vector<const char *> Engine::getRequiredExtensions() {
    uint32_t glfwExtensionsCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);

    if (engineSettings.validationLayers.size()) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Engine::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void Engine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo){
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
}

void Engine::initInstance() {
    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = engineSettings.applicationName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&createInfo.enabledExtensionCount);

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
    VkValidationFeaturesEXT features = {};

    if (engineSettings.validationLayers.size()) {
        createInfo.enabledLayerCount = engineSettings.validationLayers.size();
        createInfo.ppEnabledLayerNames = engineSettings.validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);

        if (engineSettings.validationExtentions.size()) {
            features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            features.enabledValidationFeatureCount = engineSettings.validationExtentions.size();
            features.pEnabledValidationFeatures = engineSettings.validationExtentions.data();
            debugCreateInfo.pNext = &features;
        }
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create vulkan instance.");

    if (engineSettings.validationLayers.size()) {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        populateDebugMessengerCreateInfo(debugCreateInfo);
        debugCreateInfo.pUserData = nullptr;
        debugCreateInfo.pNext = nullptr;
        debugCreateInfo.flags = 0;

        if (CreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
            throw std::runtime_error("Failed to create debug messenger.");
    }

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) throw std::runtime_error("Failed to create window surface.");
}

std::set<uint32_t> Engine::QueueFamilyIndices::families() {
    return std::set<uint32_t> { graphicsFamily.value(), presentFamily.value(), transferFamily.value() };
}

bool Engine::QueueFamilyIndices::isSupported() {
    return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
}

std::vector<const char *> Engine::QueueFamilyIndices::whatIsMissing() {
    std::vector<const char *> missingFamilies;
    if (!graphicsFamily.has_value()) missingFamilies.push_back("Graphics family queue");
    if (!presentFamily.has_value()) missingFamilies.push_back("Present family queue");
    if (!transferFamily.has_value()) missingFamilies.push_back("Transfer family queue");
    return missingFamilies;
}

std::string Engine::QueueFamilyIndices::info() {
    std::stringstream output;
    output << std::hex << "Present family: " << (presentFamily.has_value() ? std::to_string(presentFamily.value()) : "None") << std::endl;
    output << "Graphics family: " << (graphicsFamily.has_value() ? std::to_string(graphicsFamily.value()) : "None") << std::endl;
    output << "Transfer family: " << (transferFamily.has_value() ? std::to_string(transferFamily.value()) : "None") << std::endl;
    return output.str();
}

bool Engine::deviceSupportsExtensions(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

    for (const auto& extension : availableExtensions) requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

// TODO support fallback if missing a dedicated transfer queue family
Engine::QueueFamilyIndices Engine::findQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface, bool useConcurrentTransferQueue) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            if (!useConcurrentTransferQueue) indices.transferFamily = i;
        }
        if (!(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) indices.transferFamily = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) indices.presentFamily = i;

        if (indices.isSupported()) break;
        i++;
    }

    return indices;
}

Engine::SwapChainSupportDetails Engine::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool Engine::deviceSupportsSwapChain(VkPhysicalDevice device) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    return !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
}

static inline int invalidateScore(int score) { return score < 0 ? score : -score; }

void Engine::pickPhysicalDevice() {
    uint32_t deviceCount = 0;

    vulkanErrorGuard(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr), "Unable to enemerate physical devices");
    if (deviceCount == 0) throw std::runtime_error("Unable to find GPU with vulkan support.");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vulkanErrorGuard(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()), "Unable to enemerate physical devices");

    // Do we really need the device features?
    std::multimap<int, std::tuple<VkPhysicalDevice, VkPhysicalDeviceProperties, VkPhysicalDeviceFeatures, QueueFamilyIndices>> deviceInfos;
    std::transform(devices.begin(), devices.end(), std::inserter(deviceInfos, deviceInfos.begin()),
        [this](VkPhysicalDevice device) -> std::pair<int, std::tuple<VkPhysicalDevice, VkPhysicalDeviceProperties, VkPhysicalDeviceFeatures, QueueFamilyIndices>> {
            QueueFamilyIndices indices = findQueueFamilyIndices(device, surface, engineSettings.useConcurrentTransferQueue);
            VkPhysicalDeviceProperties deviceProperties;
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
            int score = deviceProperties.limits.maxImageDimension2D;
            if (!indices.isSupported() /*|| !deviceSupportsExtensions(device)*/ ||
                !deviceSupportsSwapChain(device) || !deviceFeatures.samplerAnisotropy) score = invalidateScore(score);
            return std::make_pair(score, std::make_tuple(device, deviceProperties, deviceFeatures, indices));
        });

    if (engineSettings.verbose) for(const auto& [score, details] : deviceInfos) std::cout << score << " - " << std::get<1>(details).deviceName
        << " - " << std::get<1>(details).apiVersion << std::endl;

    if (deviceInfos.rbegin()->first < 0)
        throw std::runtime_error("No suitable gpu was found on this system. (Missing feature queues, extensions, or swap support).");

    const int deviceListOffset = 0;
    physicalDevice = std::get<0>(std::next(deviceInfos.rbegin(), deviceListOffset)->second);
    physicalDeviceIndices = std::get<3>(std::next(deviceInfos.rbegin(), deviceListOffset)->second);
    // for right now just run at max availible anisotrpic filtering level for textures
    maxSamplerAnisotropy = std::get<1>(std::next(deviceInfos.rbegin(), deviceListOffset)->second).limits.maxSamplerAnisotropy;
    minUniformBufferOffsetAlignment = std::get<1>(std::next(deviceInfos.rbegin(), deviceListOffset)->second).limits.minUniformBufferOffsetAlignment;

    // TODO We should act on these values in case it doesn't support line width and make an error message rather than just try and initalize the device anyways
    lineWidthRange = {
        std::get<1>(std::next(deviceInfos.rbegin(), deviceListOffset)->second).limits.lineWidthRange[0],
        std::get<1>(std::next(deviceInfos.rbegin(), deviceListOffset)->second).limits.lineWidthRange[1]
    };
    lineWidthGranularity = std::get<1>(std::next(deviceInfos.rbegin(), deviceListOffset)->second).limits.lineWidthGranularity;

    if (engineSettings.verbose) std::cout << physicalDeviceIndices.info();
}

void Engine::setupLogicalDevice() {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    for(uint32_t queueFamily : physicalDeviceIndices.families() ) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    // deviceFeatures.wideLines = VK_TRUE;
    // Vulkan 1.2 feature that I might want to use

    VkPhysicalDeviceVulkan12Features otherFeatures {};
    otherFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    otherFeatures.pNext = nullptr;
    otherFeatures.drawIndirectCount = VK_TRUE;

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = requiredDeviceExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
    if (engineSettings.validationLayers.size()) {
        createInfo.enabledLayerCount = engineSettings.validationLayers.size();
        createInfo.ppEnabledLayerNames = engineSettings.validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    vulkanErrorGuard(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "Failed to create logical device.");

    vkGetDeviceQueue(device, physicalDeviceIndices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, physicalDeviceIndices.presentFamily.value(), 0, &presentQueue);
    vkGetDeviceQueue(device, physicalDeviceIndices.transferFamily.value(), 0, &transferQueue);
    // if (engineSettings.verbose) std::cout << std::hex << "Presentation queue: " << presentQueue << "  Graphics queue: " << graphicsQueue << "  Transfer queue: " << transferQueue << std::endl;

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    if (vmaCreateAllocator(&allocatorInfo, &memoryAllocator) != VK_SUCCESS)
        throw std::runtime_error("Unable to create memory allocator.");
}

VkSurfaceFormatKHR Engine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats)
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;

    return availableFormats[0];
}

VkPresentModeKHR Engine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes)
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Engine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Engine::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    concurrentFrames = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && concurrentFrames > swapChainSupport.capabilities.maxImageCount)
        concurrentFrames = swapChainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = concurrentFrames;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { physicalDeviceIndices.graphicsFamily.value(), physicalDeviceIndices.presentFamily.value() };
    if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    vulkanErrorGuard(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain), "Failed to create swap chain.");

    vkGetSwapchainImagesKHR(device, swapChain, &concurrentFrames, nullptr);
    swapChainImages.resize(concurrentFrames);
    vkGetSwapchainImagesKHR(device, swapChain, &concurrentFrames, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

VkImageView Engine::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    // The docs said you could just do this, I don't know if there is a downside (might be slower or something)
    viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkImageView imageView;
    vulkanErrorGuard(vkCreateImageView(device, &viewInfo, nullptr, &imageView), "Unable to create texture image view.");

    return imageView;
}

// These views are for the swap chain in case that wasn't obvious...
void Engine::createImageViews() {
    subpassImages.resize(swapChainImages.size());
    subpassImageAllocations.resize(swapChainImages.size());
    subpassImageViews.resize(swapChainImages.size());
    bgSubpassImages.resize(swapChainImages.size());
    bgSubpassImageAllocations.resize(swapChainImages.size());
    bgSubpassImageViews.resize(swapChainImages.size());
    swapChainImageViews.resize(swapChainImages.size());
    static bool notFirstCall;

    for (int i = 0; i < swapChainImages.size(); i++) {
        VkImageCreateInfo imageInfo {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapChainExtent.width;
        imageInfo.extent.height = swapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        // I am not sure about this one
        imageInfo.format = swapChainImageFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo imageAllocCreateInfo = {};
        imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        if (vmaCreateImage(memoryAllocator, &imageInfo, &imageAllocCreateInfo, &subpassImages[i], &subpassImageAllocations[i], nullptr) != VK_SUCCESS)
            throw std::runtime_error("Unable to create attachment images.");
        
        if (vmaCreateImage(memoryAllocator, &imageInfo, &imageAllocCreateInfo, &bgSubpassImages[i], &bgSubpassImageAllocations[i], nullptr) != VK_SUCCESS)
            throw std::runtime_error("Unable to create attachment images.");


        // I cant even do this here because with have no commandbuffers yet
        // transitionImageLayout(subpassImages[i], swapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        if (notFirstCall) {
            VkCommandBuffer commandBuffer = beginSingleTimeCommands();

            VkImageMemoryBarrier subpassBarrier {};
            subpassBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            subpassBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            subpassBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            subpassBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            subpassBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            subpassBarrier.image = subpassImages[i];
            subpassBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            subpassBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subpassBarrier.subresourceRange.baseMipLevel = 0;
            subpassBarrier.subresourceRange.levelCount = 1;
            subpassBarrier.subresourceRange.baseArrayLayer = 0;
            subpassBarrier.subresourceRange.layerCount = 1;
            subpassBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            subpassBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            VkImageMemoryBarrier bgBarrier {};
            bgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            bgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            bgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            bgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bgBarrier.image = bgSubpassImages[i];
            bgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            bgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bgBarrier.subresourceRange.baseMipLevel = 0;
            bgBarrier.subresourceRange.levelCount = 1;
            bgBarrier.subresourceRange.baseArrayLayer = 0;
            bgBarrier.subresourceRange.layerCount = 1;
            bgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            bgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            std::array<VkImageMemoryBarrier, 2> barriers = { subpassBarrier, bgBarrier };

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                barriers.size(), barriers.data()
            );

            endSingleTimeCommands(commandBuffer);
        }

        VkImageViewCreateInfo imageViewInfo {};

        subpassImageViews[i] = createImageView(subpassImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        bgSubpassImageViews[i] = createImageView(bgSubpassImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    notFirstCall = true;
}

void Engine::destroyImageViews() {
    for(int i = 0; i < concurrentFrames; i++) {
        vkDestroyImageView(device, subpassImageViews[i], nullptr);
        vmaDestroyImage(memoryAllocator, subpassImages[i], bgSubpassImageAllocations[i]);
        vkDestroyImageView(device, bgSubpassImageViews[i], nullptr);
        vmaDestroyImage(memoryAllocator, bgSubpassImages[i], subpassImageAllocations[i]);
    }
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
}

VkFormat Engine::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) return format;
    }
    throw std::runtime_error("Failed to find supported format.");
}

VkFormat Engine::findDepthFormat() {
    return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void Engine::createRenderPass() {
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment {};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription subpassAttachment {};
    subpassAttachment.format = swapChainImageFormat;
    subpassAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    subpassAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    subpassAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    subpassAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    subpassAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    subpassAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    subpassAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription iconAttachment {};
    iconAttachment.format = swapChainImageFormat;
    iconAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    iconAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    iconAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    iconAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    iconAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    iconAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    iconAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference subpass0color = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference subpass0depth = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    std::array<VkSubpassDescription, 3> subpasses {};
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = &subpass0color;
    subpasses[0].pDepthStencilAttachment = &subpass0depth;
    subpasses[0].pResolveAttachments = nullptr;

    VkAttachmentReference subpass1color = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount = 1;
    subpasses[1].pColorAttachments = &subpass1color;
    subpasses[1].pDepthStencilAttachment = nullptr;
    subpasses[1].pResolveAttachments = nullptr;
    subpasses[1].inputAttachmentCount = 0;
    subpasses[1].pInputAttachments = nullptr;

    VkAttachmentReference subpass2color = {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference subpass2inputs[2] = {
        { 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
    };

    subpasses[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[2].colorAttachmentCount = 1;
    subpasses[2].pColorAttachments = &subpass2color;
    subpasses[2].pDepthStencilAttachment = &subpass0depth;
    subpasses[2].pResolveAttachments = nullptr;
    subpasses[2].inputAttachmentCount = 2;
    subpasses[2].pInputAttachments = subpass2inputs;
    // Can we just reuse the depth attachment?

    std::array<VkAttachmentDescription, 4> attachments = { subpassAttachment, depthAttachment, colorAttachment, iconAttachment };
    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = subpasses.size();;
    renderPassInfo.pSubpasses = subpasses.data();

    std::array<VkSubpassDependency, 4> dependencies {};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstSubpass = 1;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[1].srcAccessMask = 0;
    dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependencies[2].srcSubpass = 0;
    dependencies[2].dstSubpass = 2;
    dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependencies[3].srcSubpass = 1;
    dependencies[3].dstSubpass = 2;
    dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[3].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[3].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = dependencies.size();
    renderPassInfo.pDependencies = dependencies.data();

    vulkanErrorGuard(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass), "Failed to create render pass.");
}

void Engine::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 4;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding shadowMapBinding {};
    shadowMapBinding.binding = 2;
    shadowMapBinding.descriptorCount = 1;
    shadowMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowMapBinding.pImmutableSamplers = nullptr;
    shadowMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding lightingBinding {};
    lightingBinding.binding = 3;
    lightingBinding.descriptorCount = 1;
    lightingBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    lightingBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding skyboxSamplerBinding {};
    skyboxSamplerBinding.binding = 4;
    skyboxSamplerBinding.descriptorCount = 1;
    skyboxSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    skyboxSamplerBinding.pImmutableSamplers = nullptr;
    skyboxSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 5> bindings = { uboLayoutBinding, samplerLayoutBinding, shadowMapBinding, lightingBinding, skyboxSamplerBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    vulkanErrorGuard(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &mainPass.descriptorSetLayout), "Failed to create descriptor set layout.");

    // for line drawing all we need is one ubo
    layoutInfo.bindingCount = 1;
    vulkanErrorGuard(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &mainPass.lineDescriptorLayout), "Failed to create descriptor set layout.");

    VkDescriptorSetLayoutBinding hudColorInput {};
    hudColorInput.binding = 0;
    hudColorInput.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    hudColorInput.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    hudColorInput.descriptorCount = 1;

    VkDescriptorSetLayoutBinding hudIconInput {};
    hudIconInput.binding = 1;
    hudIconInput.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    hudIconInput.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    hudIconInput.descriptorCount = 1;

    VkDescriptorSetLayoutBinding hudTextures {};
    hudTextures.binding = 2;
    hudTextures.descriptorCount = engineSettings.maxHudTextures;
    hudTextures.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    hudTextures.pImmutableSamplers = nullptr;
    hudTextures.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    std::array<VkDescriptorSetLayoutBinding, 3> hudBindings = { hudColorInput, hudIconInput, hudTextures };
    VkDescriptorSetLayoutCreateInfo hudLayout {};
    hudLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    hudLayout.bindingCount = hudBindings.size();
    hudLayout.pBindings = hudBindings.data();

    vulkanErrorGuard(vkCreateDescriptorSetLayout(device, &hudLayout, nullptr, &hudDescriptorLayout), "Failed to create hud descriptor set layout.");
}

VkShaderModule Engine::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    vulkanErrorGuard(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule), "Failed to create shader module.");

    return shaderModule;
}

void Engine::createGraphicsPipelines() {
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");
    auto hudVertShaderCode = readFile("shaders/hud_vert.spv");
    auto hudFragShaderCode = readFile("shaders/hud_frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
    VkShaderModule hudVertShaderModule = createShaderModule(hudVertShaderCode);
    VkShaderModule hudFragShaderModule = createShaderModule(hudFragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo hudVertShaderStageInfo {};
    hudVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    hudVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    hudVertShaderStageInfo.module = hudVertShaderModule;
    hudVertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo hudFragShaderStageInfo {};
    hudFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    hudFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    hudFragShaderStageInfo.module = hudFragShaderModule;
    hudFragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    VkPipelineShaderStageCreateInfo hudShaders[] = { hudVertShaderStageInfo, hudFragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    // the validation is complaining about this, but I can't see why (as far as I can see the validation is just reporting something which is incorrect)
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &mainPass.descriptorSetLayout;

    VkPushConstantRange pushConstantRange;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    vulkanErrorGuard(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout), "Failed to create pipeline layout.");

    pipelineLayoutInfo.pSetLayouts = &mainPass.descriptorSetLayout;

    vulkanErrorGuard(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &linePipelineLayout), "Failed to create pipeline layout.");

    VkPipelineDepthStencilStateCreateInfo depthStencil {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;

    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    pipelineInfo.pDepthStencilState = &depthStencil;

    // vulkanErrorGuard(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &graphicsPipelines[0]), "Failed to create graphics pipeline.");
    vulkanErrorGuard(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipelines[GP_WORLD]), "Failed to create graphics pipeline.");

    colorBlendAttachment.blendEnable = VK_FALSE;
    pipelineInfo.subpass = 1;

    vulkanErrorGuard(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipelines[GP_BG]), "Failed to create graphics pipeline.");

    // I think I need a new layout for the line drawing, a way to effeciently express the lines to the 

    colorBlendAttachment.blendEnable = VK_FALSE;
    pipelineInfo.subpass = 0;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    pipelineInfo.layout = linePipelineLayout;
    vulkanErrorGuard(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipelines[GP_LINES]), "Failed to create graphics pipeline.");

    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // turn off depth stenciling for drawing the hud
    // I am going to try and reuse the depth resource
    // pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.subpass = 2;

    VkPipelineVertexInputStateCreateInfo hudVertexInputInfo {};
    hudVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto hudBindingDescription = GuiVertex::getBindingDescription();
    auto hudAttributeDescriptions = GuiVertex::getAttributeDescriptions();

    hudVertexInputInfo.vertexBindingDescriptionCount = 1;
    hudVertexInputInfo.vertexAttributeDescriptionCount = hudAttributeDescriptions.size();
    hudVertexInputInfo.pVertexBindingDescriptions = &hudBindingDescription;
    hudVertexInputInfo.pVertexAttributeDescriptions = hudAttributeDescriptions.data();

    pipelineInfo.pVertexInputState = &hudVertexInputInfo;

    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = hudShaders;

    VkPipelineLayoutCreateInfo hudLayoutInfo {};
    hudLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    hudLayoutInfo.setLayoutCount = 1;
    hudLayoutInfo.pSetLayouts = &hudDescriptorLayout;

    // This way I don't have to worry about which is the front side of the triangles for the hud, we wouldnt want anything culled anyways
    rasterizer.cullMode = VK_CULL_MODE_NONE;

    // I am going to use this soon for drawing pathing lines and stuff in the world
    // VkDynamicState dynamicStates[] = {
    //     VK_DYNAMIC_STATE_LINE_WIDTH
    // };

    // VkPipelineDynamicStateCreateInfo dynamicState {};
    // dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    // dynamicState.dynamicStateCount = 1;
    // dynamicState.pDynamicStates = dynamicStates;

    // pipelineInfo.pDynamicState = &dynamicState;

    VkPushConstantRange hudPushConstantRange;
    hudPushConstantRange.offset = 0;
    hudPushConstantRange.size = sizeof(GuiPushConstant);
    // Why need fragment stage access here???? (I don't for my needs, but the validation is complaining, (it is like mixing up the subpasses or something))
    hudPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    hudLayoutInfo.pushConstantRangeCount = 1;
    hudLayoutInfo.pPushConstantRanges = &pushConstantRange;

    vulkanErrorGuard(vkCreatePipelineLayout(device, &hudLayoutInfo, nullptr, &hudPipelineLayout), "Failed to create pipeline layout.");

    pipelineInfo.layout = hudPipelineLayout;

    // vulkanErrorGuard(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &graphicsPipelines[1]), "Failed to create graphics pipeline.");
    vulkanErrorGuard(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipelines[GP_HUD]), "Failed to create graphics pipeline.");

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, hudFragShaderModule, nullptr);
    vkDestroyShaderModule(device, hudVertShaderModule, nullptr);
}

// Remember that all the command buffers from a pool must be accessed from the same thread
void Engine::createCommandPools() {
    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = physicalDeviceIndices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vulkanErrorGuard(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool), "Failed to create command pool.");
    // The gui pool is exactly the same, but it is just accessed from another thread so we need another pool
    vulkanErrorGuard(vkCreateCommandPool(device, &poolInfo, nullptr, &guiCommandPool), "Failed to create command pool.");

    if (engineSettings.useConcurrentTransferQueue) {
        poolInfo.flags = 0;
        poolInfo.queueFamilyIndex = physicalDeviceIndices.transferFamily.value();

        vulkanErrorGuard(vkCreateCommandPool(device, &poolInfo, nullptr, &transferCommandPool), "Failed to create transfer command pool.");
    }
}

uint32_t Engine::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    if(engineSettings.extremelyVerbose)
        std::cout << "Most calls to this function should be elliminated in favor of using the vma allocator." << std::endl
            << boost::stacktrace::stacktrace() << std::endl;

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) if (typeFilter & (1 << i) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) return i;

    throw std::runtime_error("Failed to find suitable memory type.");
}

// TODO Really all calls to this should be replaced with vmaCreateImage
void Engine::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLevels) {

    // std::cout << std::dec << "Creating image with mip levels of " << mipLevels << std::endl;
    VkImageCreateInfo imageInfo {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    // should the transfer queue transfer it?
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vulkanErrorGuard(vkCreateImage(device, &imageInfo, nullptr, &image), "Failed to create image.");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    vulkanErrorGuard(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory), "Failed to allocate image memory.");

    vkBindImageMemory(device, image, imageMemory, 0);
}

// This creates a command buffer for the graphics queue in the default command pool
VkCommandBuffer Engine::beginSingleTimeCommands() {
    return beginSingleTimeCommands(commandPool);
}

// We can use this to create a command buffer on other queues (namely the tranfer queue) in the default command buffer
VkCommandBuffer Engine::beginSingleTimeCommands(VkCommandPool pool) {
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Engine::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    endSingleTimeCommands(commandBuffer, commandPool, graphicsQueue);
}

void Engine::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool pool, VkQueue queue) {
    // std::cout << "using pool: " << std::hex << pool << "   and queue: " << queue << std::endl;
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
}

bool Engine::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

// Note that this should not be done with the tranfer queue (the validation layer complains that you are using the queue incorrectly)
void Engine::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
    // std::cout << "Using transition mip levels of " << mipLevels << std::endl;
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else throw std::invalid_argument("Unsupported layout transition.");

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void Engine::createDepthResources() {
    depthImages.resize(swapChainImages.size());
    depthImageAllocations.resize(swapChainImages.size());
    depthImageViews.resize(swapChainImages.size());

    VkFormat depthFormat = findDepthFormat();

    depthDump.makeDump = false;
    depthDump.writePending = false;
    if (DEPTH_DEBUG_IMAGE_USAGE) {

        VkBufferCreateInfo debuggingBufferInfo = {};
        debuggingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        debuggingBufferInfo.size = swapChainExtent.width * swapChainExtent.height * 4;
        debuggingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo debuggingAllocationInfo = {};
        debuggingAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
        debuggingAllocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo debuggingBufferAl = {};

        if (vmaCreateBuffer(memoryAllocator, &debuggingBufferInfo, &debuggingAllocationInfo, &depthDump.buffer, &depthDump.allocation, &depthDump.allocationInfo) != VK_SUCCESS)
            throw std::runtime_error("Unable to allocate memory for main depth dump buffer.");
    }

    for (int i = 0; i < swapChainImages.size(); i++) {
        VkImageCreateInfo imageInfo {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapChainExtent.width;
        imageInfo.extent.height = swapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | DEPTH_DEBUG_IMAGE_USAGE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo imageAllocCreateInfo = {};
        imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateImage(memoryAllocator, &imageInfo, &imageAllocCreateInfo, &depthImages[i], &depthImageAllocations[i], nullptr);
        
        depthImageViews[i] = createImageView(depthImages[i], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
        transitionImageLayout(depthImages[i], depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }
}

void Engine::dumpDepthBuffer(const VkCommandBuffer& buffer, int index) {
    // I really dont want to garble up the buffer (I should only ever hit this due to a programming error)
    assert(!depthDump.writePending);

    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = depthImages[index];

    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // not sure about src mask ????
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    // I do know that the dst access mask is right
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    // also no clue about stage masks??? I am continually confused by these things
    vkCmdPipelineBarrier(
        buffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = swapChainExtent.width;
    region.imageExtent.height = swapChainExtent.height;
    region.imageExtent.depth = 1;

    vkCmdCopyImageToBuffer(buffer, depthImages[index], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, depthDump.buffer, 1, &region);

    // I don't think we have any reason to need this memory barrier in place (we get the dump and the end of the render pass and then we just dont care anymore)
    // barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    // barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    // barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    // Still no clue about stage masks
    // vkCmdPipelineBarrier(
    //     buffer,
    //     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    //     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    //     0,
    //     0, nullptr,
    //     0, nullptr,
    //     1, &barrier
    // );

    depthDump.writePending = true;
    depthDump.makeDump = false;
}

std::array<int, 256>& Engine::logDepthBufferToFile(std::array<int, 256>& depthHistogram, const char *filename) {
    assert(depthDump.writePending);

    memset(depthHistogram.data(), 0, sizeof(int) * depthHistogram.size());

    // idk about single channle png images?
    unsigned char *buf = new unsigned char[swapChainExtent.width * swapChainExtent.height];
    float *data = static_cast<float *>(depthDump.allocation->GetMappedData());

    for (int y = 0; y < swapChainExtent.height; y++) {
        for (int x = 0; x < swapChainExtent.width; x++) {
            unsigned char value = static_cast<unsigned char>(data[y * swapChainExtent.width + x] * 255.0f);
            buf[y * swapChainExtent.width + x] = value;
            depthHistogram[value] += 1;
        }
    }

    stbi_write_png(filename, swapChainExtent.width, swapChainExtent.height, 1, buf, swapChainExtent.width);

    delete buf;

    depthDump.writePending = false;

    return depthHistogram;
}

void Engine::destroyDepthResources() {
    for(int i = 0; i < depthImages.size(); i++) {
        vkDestroyImageView(device, depthImageViews[i], nullptr);
        vmaDestroyImage(memoryAllocator, depthImages[i], depthImageAllocations[i]);
    }
}

void Engine::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 4> attachments = {
            subpassImageViews[i],
            depthImageViews[i],
            swapChainImageViews[i],
            bgSubpassImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        vulkanErrorGuard(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]), "Failed to create framebuffer.");
    }
}

// TODO Elliminate this in favor of using vmaCreateBuffer
void Engine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
    VkDeviceMemory& bufferMemory, bool concurrent, std::vector<uint32_t> queues) {

    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = concurrent ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    if (concurrent) {
        bufferInfo.queueFamilyIndexCount = queues.size();
        bufferInfo.pQueueFamilyIndices = queues.data();
    }

    vulkanErrorGuard(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer), "Failed to create buffer.");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    vulkanErrorGuard(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory), "Failed to allocate buffer memory.");

    vkBindBufferMemory(device, buffer, bufferMemory, 0);

    // std::cout << std::hex << "Buffer: " << buffer << "\n" << boost::stacktrace::stacktrace() << std::endl;
}

void Engine::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = engineSettings.useConcurrentTransferQueue ? beginSingleTimeCommands(transferCommandPool) : beginSingleTimeCommands();

    VkBufferImageCopy region {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    if (engineSettings.useConcurrentTransferQueue) endSingleTimeCommands(commandBuffer, transferCommandPool, transferQueue);
    else endSingleTimeCommands(commandBuffer);
}

void Engine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool pool, VkQueue queue) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(pool);

    VkBufferCopy copyRegion {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer, pool, queue);
}

void Engine::createModelBuffers() {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // Vertex buffer
    VkDeviceSize bufferSize = currentScene->vertexBuffer.size() * sizeof(Vertex);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, currentScene->vertexBuffer.data(), bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory,
        engineSettings.useConcurrentTransferQueue, { physicalDeviceIndices.graphicsFamily.value(), physicalDeviceIndices.transferFamily.value() });

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize,
        engineSettings.useConcurrentTransferQueue ? transferCommandPool : commandPool,
        engineSettings.useConcurrentTransferQueue ? transferQueue : graphicsQueue);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    // Index buffer
    bufferSize = currentScene->indexBuffer.size() * sizeof(uint32_t);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, currentScene->indexBuffer.data(), bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory,
        engineSettings.useConcurrentTransferQueue, { physicalDeviceIndices.graphicsFamily.value(), physicalDeviceIndices.transferFamily.value() });

    copyBuffer(stagingBuffer, indexBuffer, bufferSize,
        engineSettings.useConcurrentTransferQueue ? transferCommandPool : commandPool,
        engineSettings.useConcurrentTransferQueue ? transferQueue : graphicsQueue);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Engine::createHudBuffers() {
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = 50 * sizeof(GuiVertex);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo bufferAllocationInfo = {};
    bufferAllocationInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    bufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if (vmaCreateBuffer(memoryAllocator, &bufferInfo, &bufferAllocationInfo, &hudBuffer, &hudAllocation, nullptr) !=VK_SUCCESS)
        throw std::runtime_error("Unable to allocate memory for hud buffer.");
}

void Engine::initVulkan() {
    enableValidationLayers();
    initInstance();
    pickPhysicalDevice();
    setupLogicalDevice();


    // VkPipelineCacheCreateInfo pipelineCacheCI = {};
    // pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    // pipelineCacheCI.flags = VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT_EXT;
    // pipelineCacheCI.pNext = nullptr;
    // pipelineCacheCI.pInitialData = nullptr;
    // pipelineCacheCI.initialDataSize = 0;

    // vulkanErrorGuard(vkCreatePipelineCache(device, &pipelineCacheCI, nullptr, &pipelineCache), "Unable to create pipeline cache.");

    createSwapChain();
    framebufferResized = false;
    createImageViews();
    createRenderPass();

    createDescriptorSetLayout();
    createGraphicsPipelines();
    createCommandPools();

    // this needs to match the size of the swapchain rather than the command buffer actually
    // Lets try using the same descriptor layout as the main render pass to make the code more simple
    createShadowResources(true);

    createDepthResources();
    createFramebuffers();

    // this sets the currentScene pointer
    loadDefaultScene();

    createHudBuffers();
}

void Engine::init() {
    initWidow();
    initVulkan();
}

// TODO Name this better (What is this calculated value called anyways?)
static float whichSideOfPlane(glm::vec3 n, float d, glm::vec3 x) {
    return glm::dot(n, x) + d;
}

// Note that this is signed and N HAS TO BE NORMALIZED
static float distancePointToPlane(glm::vec3 n, float d, glm::vec3 x) {
    return (glm::dot(n, x) + d) / glm::l2Norm(n);
}

// creates the frustum, but not actually a frustum, bounding planes (passing the straffing vector saves having to recalculate it)
static std::array<std::pair<glm::vec3, float>, 5> boundingPlanes(glm::vec3 pos, glm::vec3 strafing, glm::vec3 normedPointing, glm::vec3 r1, glm::vec3 r2) {
    std::array<std::pair<glm::vec3, float>, 5> ret;
    
    auto n = normalize(cross(strafing, r1));
    ret[0] = { n, -distancePointToPlane(n, 0, pos) };

    n = normalize(cross(cross(strafing, normedPointing), r1));
    ret[1] = { n, -distancePointToPlane(n, 0, pos) };

    n = normalize(cross(strafing, r2));
    ret[2] = { n, -distancePointToPlane(n, 0, pos) };

    n = normalize(cross(cross(strafing, normedPointing), r2));
    ret[3] = { n, -distancePointToPlane(n, 0, pos) };

    ret[4] = { normedPointing, -distancePointToPlane(normedPointing, 0, pos) };

    return ret;
}

inline std::pair<float, float> Engine::normedDevice(float x, float y) {
    return { x / framebufferSize.width * 2.0f - 1.0, y / framebufferSize.height * 2.0f - 1.0f };
}

inline glm::vec3 Engine::raycast(float x, float y, glm::mat4 inverseProjection, glm::mat4 inverseView) {
    return raycastDevice(normedDevice(x, y), inverseProjection, inverseView);
}

glm::vec3 Engine::raycastDevice(float normedDeviceX, float normedDeviceY, glm::mat4 inverseProjection, glm::mat4 inverseView) {
    auto homogeneousRay = glm::vec4(-normedDeviceX, -normedDeviceY, 1.0f, 1.0f);
    auto eyeRay = inverseProjection * homogeneousRay;
    eyeRay.z = 1.0f;
    eyeRay.w = 0.0f;
    auto worldRay = inverseView * eyeRay;
    glm::vec3 ray = { worldRay.x, worldRay.y, worldRay.z };
    return -ray;
}

inline glm::vec3 Engine::raycastDevice(std::pair<float, float> normedDevice, glm::mat4 inverseProjection, glm::mat4 inverseView) {
    return raycastDevice(normedDevice.first, normedDevice.second, inverseProjection, inverseView);
}

// So many silly lines of code in this function
void Engine::handleInput() {
    Gui::GuiMessage message;
    while(gui->guiMessages.pop(message)) {
        if(message.signal == Gui::GUI_SOMETHING) {
            currentScene->state.instances.erase(currentScene->state.instances.begin() + 1, currentScene->state.instances.end());
        }
    }


    double x, y;
    glfwGetCursorPos(window, &x, &y);
    float deltaX = (lastMousePosition.x - x) / framebufferSize.width;
    float deltaY = (lastMousePosition.y - y) / framebufferSize.height;

    auto inverseProjection = glm::inverse(pushConstants.projection);
    auto inverseView = glm::inverse(pushConstants.view);
    auto mouseNormed = normedDevice(x, y);
    auto mouseRay = raycast(x, y, inverseProjection, inverseView);
    auto mouseRayNormed = normalize(mouseRay);
    // hudConstants.dragBox[1] = { mouseNormed.first, mouseNormed.second };

    lastMousePosition.normedX = mouseNormed.first;
    lastMousePosition.normedY = mouseNormed.second;

    // Do I want this?
    for (int i = 0; i < 8; i++) {
        if (mouseButtonsPressed[i]) {
            mouseButtonsPressed[i] = false;
        }
    }
    KeyEvent keyEvent;
    while(keyboardInput.pop(keyEvent)) {
        if (keyEvent.action == GLFW_PRESS) {
            keysPressed[keyEvent.key] = true;

            if (keyEvent.key == GLFW_KEY_T) {
                // shadow.makeSnapshot = true;
                std::cout << "Gui id: " << gui->pushConstant()->guiID << std::endl;
            }
            if (keyEvent.key == GLFW_KEY_Y) {
                GuiCommandData *what3 = new GuiCommandData();
                what3->childIndices.push(1);
                // what3->childIndices.push(1);
                gui->submitCommand({ Gui::GUI_REMOVE, what3 });
            }
            if (keyEvent.key == GLFW_KEY_B) {
                std::raise(SIGTRAP);
            }
            if (keyEvent.key == GLFW_KEY_O) {
                GuiCommandData *what = new GuiCommandData();
                // what->childIndices.push(0); // Don't actually push anything rn since we have no root node by default
                what->component = new GuiComponent(gui, false, 0x0000ff40, { -1.0, 0.7 }, { 1.0, 1.0 }, 1);
                gui->submitCommand({ Gui::GUI_ADD, what });
                GuiCommandData *what2 = new GuiCommandData();
                // what2->childIndices.push(0); // Don't actually push anything rn since we have no root node by default
                what2->component = new GuiLabel(gui, "clear", 0x000000ff, 0x60609940, { -0.4, 0.8 }, 0.05, 2);
                gui->submitCommand({ Gui::GUI_ADD, what2 });
            }
        } else if (keyEvent.action == GLFW_RELEASE) {
            keysPressed[keyEvent.key] = false;
        }
    }
    // TODO This whole thing could be more effeciently calculated (when I feel like being clever I will come back and fix it)
    cammera.pointing = normalize(cammera.position - cammera.target);
    auto pointingLength = length(cammera.position - cammera.target);
    cammera.strafing = normalize(cross(cammera.pointing, { 0.0f, 0.0f, 1.0f }));
    cammera.fowarding = normalize(cross(cammera.strafing, { 0.0f, 0.0f, 1.0f }));
    cammera.heading = normalize(cross(cammera.pointing, cammera.strafing));
    // auto orbit = glm::angleAxis(glm::radians(0.1f), glm::vec3({ 0.0f, 0.0f, 1.0f }));
    // auto reverseOrbit = glm::angleAxis(glm::radians(359.9f), glm::vec3({ 0.0f, 0.0f, 1.0f }));
    // auto orbitMatrix = glm::toMat3(orbit);
    // auto reverseOrbitMatrix = glm::toMat3(reverseOrbit);
    // auto tilt = glm::angleAxis(glm::radians(0.1f), strafing);
    // auto reverseTilt = glm::angleAxis(glm::radians(359.9f), strafing);
    // auto tiltMatrix = glm::toMat3(tilt);
    // auto reverseTiltMatrix = glm::toMat3(reverseTilt);
    float planeIntersectionDenominator = glm::dot(mouseRay, { 0.0f, 0.0f, 1.0f });
    float planeDist;
    glm::vec3 planeIntersection;
    if (planeIntersectionDenominator == 0) {
        planeIntersection = glm::vec3(0.0);
    } else {
        planeDist = - (glm::dot(cammera.position, { 0.0f, 0.0f, 1.0f }) + 0) / planeIntersectionDenominator;
        planeIntersection = cammera.position + mouseRay * planeDist;
    }
    if (scrollAmount != 0.0f) {
        // Scroll to cammera target
        // cammera.position += scrollAmount / 5.0f * pointing;
        // if (glm::distance(cammera.target, cammera.position) > cammera.maxZoom2) cammera.position = pointing * cammera.maxZoom2 + cammera.target;
        // if (glm::distance(cammera.target, cammera.position) < cammera.minZoom2) cammera.position = pointing * cammera.minZoom2 + cammera.target;
        // scrollAmount = 0.0f;
        // scrollToCursor
        auto deltaPos = -scrollAmount / 5.0f * mouseRayNormed;
        auto deltaTarget = normalize(planeIntersection - cammera.target) * (length(deltaPos) * length(planeIntersection - cammera.target) / planeDist);
        auto newTar = cammera.target + deltaTarget * sgn(-scrollAmount);
        auto newPos = cammera.position + deltaPos;
        auto newDelta = length2(newTar - newPos);
        if (newDelta > cammera.minZoom2 && newDelta < cammera.maxZoom2) {
            cammera.target = newTar;
            cammera.position = newPos;
        }

        scrollAmount = 0.0f;
    }

    if (mouseAction == MOUSE_DRAGGING) {
        gui->setDragBox(dragStartDevice, mouseNormed);
    }

    MouseEvent mouseEvent;
    while (mouseInput.pop(mouseEvent)) {
        if (mouseEvent.action == GLFW_PRESS && mouseEvent.mods & GLFW_MOD_SHIFT && mouseEvent.button == GLFW_MOUSE_BUTTON_MIDDLE) {
            mouseAction = MOUSE_PANNING;
        } else if (mouseEvent.action == GLFW_PRESS && mouseEvent.button == GLFW_MOUSE_BUTTON_MIDDLE) {
            mouseAction = MOUSE_ROTATING;
        }
        if (mouseEvent.action == GLFW_RELEASE && mouseEvent.button == GLFW_MOUSE_BUTTON_MIDDLE) {
            mouseAction = MOUSE_NONE;
        }
        if (mouseAction == MOUSE_NONE && mouseEvent.action == GLFW_PRESS && mouseEvent.button == GLFW_MOUSE_BUTTON_LEFT) {
            mouseAction = MOUSE_DRAGGING;
            // Unless the framerate is crap this should be almost identical to the mouseRay
            // dragStartRay = raycast(mouseEvent.x, mouseEvent.y, inverseProjection, inverseView);
            dragStartRay = mouseRayNormed;
            dragStartDevice = mouseNormed;
            // hudConstants.dragBox[0] = { mouseNormed.first, mouseNormed.second };
        }
        if (mouseAction == MOUSE_DRAGGING && mouseEvent.action == GLFW_RELEASE && mouseEvent.button == GLFW_MOUSE_BUTTON_LEFT) {
            gui->setDragBox({ 0.0f, 0.0f }, { 0.0f, 0.0f });
            mouseAction = MOUSE_NONE;
            // std::cout << "Dragged box (" << dragStartDevice.first << ":" << dragStartDevice.second << " - " 
            //     << mouseNormed.first << ":" << mouseNormed.second << "):" << std::endl;
            auto planes = boundingPlanes(cammera.position, cammera.strafing, cammera.pointing, dragStartRay, mouseRayNormed);
            // auto planes = boundingPlanes(cammera.position, strafing, pointing, dragStartRay, raycast(mouseEvent.x, mouseEvent.y, inverseProjection, inverseView));
            // for(const auto& plane : planes) {
            //     std::cout << "\t" << plane.first << " --- " << plane.second << std::endl;
            // }
            for (int i = 0; i < currentScene->state.instances.size(); i++) {
                (currentScene->state.instances.data() + i)->highlight = 
                    whichSideOfPlane(planes[4].first, planes[4].second - cammera.minClip, (currentScene->state.instances.data() + i)->position) < 0 &&
                    whichSideOfPlane(planes[0].first, planes[0].second, (currentScene->state.instances.data() + i)->position) < 0 !=
                        whichSideOfPlane(planes[2].first, planes[2].second, (currentScene->state.instances.data() + i)->position) < 0 &&
                    whichSideOfPlane(planes[1].first, planes[1].second, (currentScene->state.instances.data() + i)->position) < 0 !=
                        whichSideOfPlane(planes[3].first, planes[3].second, (currentScene->state.instances.data() + i)->position) < 0
                    // To only select things as far as the cammera drawing distance
                    // && whichSideOfPlane(planes[4].first, planes[4].second - cammera.maxClip, (currentScene->state.instances + i)->position) > 0
                    ;
            }
        }
        if (mouseEvent.action == GLFW_PRESS && mouseEvent.button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (planeIntersectionDenominator == 0) {
                std::cout << "orthognal" << std::endl;
            } else {
                currentScene->addInstance(0, { planeIntersection.x, planeIntersection.y, planeIntersection.z }, {
                    static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / M_PI * 2)),
                    static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / M_PI * 2)),
                    static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / M_PI * 2)) });
            }
        }
    }

    if (keysPressed[GLFW_KEY_RIGHT]) {
        cammera.position -= cammera.strafing / 600.0f;
        cammera.target -= cammera.strafing / 600.0f;
    }
    if (keysPressed[GLFW_KEY_LEFT]) {
        cammera.position += cammera.strafing / 600.0f;
        cammera.target += cammera.strafing / 600.0f;
    }
    if (keysPressed[GLFW_KEY_DOWN]) {
        cammera.position -= cammera.fowarding / 600.0f;
        cammera.target -= cammera.fowarding / 600.0f;
    }
    if (keysPressed[GLFW_KEY_UP]) {
        cammera.position += cammera.fowarding / 600.0f;
        cammera.target += cammera.fowarding / 600.0f;
    }

    if (keysPressed[GLFW_KEY_U]) {
        std::cout << "Mouse normed vector: " << mouseRayNormed << std::endl;
        std::cout << "Cammera position: " << cammera.position << std::endl;
    }

    if (mouseAction == MOUSE_PANNING) {
        cammera.position -= cammera.strafing * 4.0f * deltaX * pointingLength / 5.2;
        cammera.target -= cammera.strafing * 4.0f * deltaX * pointingLength / 5.2;
        cammera.position -= cammera.fowarding * 4.0f * deltaY / (cammera.position.z / pointingLength) * pointingLength / 5.2;
        cammera.target -= cammera.fowarding * 4.0f * deltaY / (cammera.position.z / pointingLength) * pointingLength / 5.2;
    } else if (mouseAction == MOUSE_ROTATING) {
        if (deltaY != 0.0f) {
            auto mouseTilt = glm::angleAxis(glm::radians(0 - deltaY * 400.0f) / 2.0f, cammera.strafing);
            auto mouseTiltMatrix = glm::toMat3(mouseTilt);
            auto newPosition = mouseTiltMatrix * (cammera.position - cammera.target) + cammera.target;
            if (/*(newPosition.z - cammera.target.z) > cammera.gimbleStop && */(newPosition.x - cammera.target.x) * (newPosition.x - cammera.target.x) +
                (newPosition.y - cammera.target.y) * (newPosition.y - cammera.target.y) > cammera.gimbleStop) {
                    cammera.position = newPosition;
            }
        }
        if (deltaX != 0.0f) {
            auto mouseOrbit = glm::angleAxis(glm::radians(deltaX * 400.0f) / 2.0f, glm::vec3({ 0.0f, 0.0f, 1.0f }));
            auto mouseOrbitMatrix = glm::toMat3(mouseOrbit);
            cammera.position = mouseOrbitMatrix * (cammera.position - cammera.target) + cammera.target;
        }
    }

    // // use key presses for now
    // if (keysPressed[GLFW_KEY_A]) {
    //     cammera.position = orbitMatrix * (cammera.position - cammera.target) + cammera.target;
    // }
    // if (keysPressed[GLFW_KEY_D]) {
    //     cammera.position = reverseOrbitMatrix * (cammera.position - cammera.target) + cammera.target;
    // }
    // if (keysPressed[GLFW_KEY_W]) {
    //     auto newPosition = tiltMatrix * (cammera.position - cammera.target) + cammera.target;
    //     // avoid gimble problems
    //     if ((newPosition.x - cammera.target.x) * (newPosition.x - cammera.target.x) +
    //         (newPosition.y - cammera.target.y) * (newPosition.y - cammera.target.y) > cammera.gimbleStop) {
    //             cammera.position = newPosition;
    //     }
    // }
    // if (keysPressed[GLFW_KEY_S]) {
    //     auto newPosition = reverseTiltMatrix * (cammera.position - cammera.target) + cammera.target;
    //     if ((newPosition.z - cammera.target.z) > cammera.gimbleStop) {
    //             cammera.position = newPosition;
    //     }
    // }

    Instance *mousedOver = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for(int i = 1; i < currentScene->state.instances.size(); i++) {
        float distance;
        if(currentScene->state.instances[i].intersects(cammera.position, mouseRayNormed, distance)) {
            // !!!! potentially a pointer geting invalidated
            if (distance < minDistance) mousedOver = &currentScene->state.instances[i];
        }
        // instance.highlight() = false;
    }

    if(mousedOver) {
        // mousedOver->highlight() = true;
        glfwSetCursor(window, cursors[CursorIndex::CURSOR_SELECT]);
    } else {
        glfwSetCursor(window, cursors[CursorIndex::CURSOR_DEFAULT]);
    }

    lastMousePosition.x = x;
    lastMousePosition.y = y;
}

instanceZSorter::instanceZSorter(Scene *context)
: context(context) {}

bool instanceZSorter::operator() (int a, int b) {
    return context->state.instances[a].cammeraDistance2 > context->state.instances[b].cammeraDistance2;
}

// The index is for which descriptor index to put the lighting buffer information
// Here is the big graphics update function
void Engine::updateScene(int index) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    pushConstants.view = glm::lookAt(cammera.position, cammera.target, glm::vec3(0.0f, 0.0f, 1.0f));
    pushConstants.projection = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, cammera.minClip, cammera.maxClip);
    pushConstants.projection[1][1] *= -1;
    // TODO I should just store this in the camera struct since I need it here and in the input handling
    pushConstants.normedPointing = normalize(cammera.position - cammera.target);

    if (lightingDirty[index]) {
        lightingData.nearFar = { 0.7f, 13.0f };
        lightingData.pos = { 0.0f, 10.0f, 0.0f };
        lightingData.view = glm::lookAt(lightingData.pos, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        lightingData.proj = glm::perspective(glm::radians(45.0f), 1.0f, lightingData.nearFar.x, lightingData.nearFar.y);
        lightingData.proj[1][1] *= -1;

        shadow.constants.lightPos = lightingData.pos;
        shadow.constants.view = lightingData.view;
        shadow.constants.projection = lightingData.proj;

        updateLightingDescriptors(index, lightingData);

        lightingDirty[index] = false;
    }

    // The 0th instance is the skybox (probably shouldnt be like this though)
    currentScene->state.instances[0].position = cammera.position;

    // zSortedIcons.clear();
    // It is probably better just to reserve to avoid reallocations
    // zSortedIcons.reserve(currentScene->state.instances.size());
    for(int i = 1; i < currentScene->state.instances.size(); i++) {
        currentScene->state.instances[i].cammeraDistance2 = distance2(cammera.position, currentScene->state.instances[i].position);
        currentScene->state.instances[i].renderAsIcon = currentScene->state.instances[i].cammeraDistance2 > cammera.renderAsIcon2;
        // if (currentScene->state.instances[i].renderAsIcon) zSortedIcons.push_back(i);
    }

    // std::sort(zSortedIcons.begin(), zSortedIcons.end(), currentScene->zSorter);

    // 0 1
    // 3 2
    iconPrimative = {{
        { (-cammera.strafing + cammera.heading), { 1.0f, 1.0f, 1.0f }, { 0.0f , 0.0f }, -cammera.pointing },
        { ( cammera.strafing + cammera.heading), { 1.0f, 1.0f, 1.0f }, { 1.0f , 0.0f }, -cammera.pointing },
        { (-cammera.strafing - cammera.heading), { 1.0f, 1.0f, 1.0f }, { 1.0f , 1.0f }, -cammera.pointing },
        { (-cammera.strafing - cammera.heading), { 1.0f, 1.0f, 1.0f }, { 0.0f , 1.0f }, -cammera.pointing }
    }};
}

#if (DEPTH_DEBUG_IMAGE_USAGE == VK_IMAGE_USAGE_TRANSFER_SCR_BIT)
static std::array<int, 256> mainDepthHistogram {};
#endif

// TODO The synchornization code is pretty much nonsence, it works but is bad (it require the swap chain size to match the in flight frames count)
// The fences are for the swap chain, but they also protect the command buffers since we are tripple buffering the commands (at least on my machine)
void Engine::drawFrame() {
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to aquire swap chain image.");
    // TODO This fence is in the wrong spot, but to move it we need to make concurrentFrames numbers of shadow framebuffers first so we dont corrupt stuff
    vkWaitForFences(device, 1, inFlightFences.data() + (currentFrame + concurrentFrames - 1) % concurrentFrames, VK_TRUE, UINT64_MAX);

    // update the gui vram if the gui buffer has been rebuilt
    if (gui->rebuilt) {
        hudVertexCount = gui->updateBuffer(hudAllocation->GetMappedData(), 50);
        guiOutOfDate = false;
    }
    gui->pushConstant()->guiID = gui->idUnderPoint((GuiVertex *)hudAllocation->GetMappedData(), hudVertexCount, lastMousePosition.normedX, lastMousePosition.normedY);

    // This relies on the fence to stay synchronized
    if (shadow.debugWritePending)
        doShadowDebugWrite();
    if (depthDump.writePending)
        logDepthBufferToFile(mainDepthHistogram, "depth_map.png");
    // Wait for the previous fence
    recordCommandBuffer(commandBuffers[currentFrame], swapChainFramebuffers[imageIndex], mainPass.descriptorSets[currentFrame],
        hudDescriptorSets[currentFrame], hudBuffer, currentFrame);
    updateScene(currentFrame);
    // update the next frames uniforms
    currentScene->updateUniforms((currentFrame + 1) % concurrentFrames);

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // reset the fence to unsignalled
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vulkanErrorGuard(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]), "Failed to submit draw command buffer.");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        recreateSwapChain();
        framebufferResized = false;
    } else if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to present swap chain image.");

    currentFrame = (currentFrame + 1) % concurrentFrames;
}

void Engine::allocateLightingBuffers() {
    // uniformBuffers.resize(swapChainImages.size());
    // uniformBufferAllocations.resize(swapChainImages.size());
    // uniformSkip = calculateUniformSkipForUBO<UniformBufferObject>();
    lightingBuffers.resize(concurrentFrames);
    lightingBufferAllocations.resize(concurrentFrames);

    for (int i = 0; i < concurrentFrames; i++) {
        // bufferInfo.size = instanceCount * uniformSkip;
        // bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        // bufferAllocationInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        // bufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        // if (vmaCreateBuffer(memoryAllocator, &bufferInfo, &bufferAllocationInfo, uniformBuffers.data() + i, uniformBufferAllocations.data() + i, nullptr) != VK_SUCCESS)
        //     throw std::runtime_error("Unable to allocate memory for uniform buffers.");
        
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = sizeof(ViewProjPosNearFar);
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo bufferAllocationInfo = {};
        bufferAllocationInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        bufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        if (vmaCreateBuffer(memoryAllocator, &bufferInfo, &bufferAllocationInfo, &lightingBuffers[i], &lightingBufferAllocations[i], nullptr) != VK_SUCCESS)
            throw std::runtime_error("Unable to allocate memory for uniform lighting buffers.");
    }
}

void Engine::destroyLightingBuffers() {
    for (int i = 0; i < concurrentFrames; i++) {
        vmaDestroyBuffer(memoryAllocator, lightingBuffers[i], lightingBufferAllocations[i]);
    }
}

void Engine::runCurrentScene() {
    // We only need to create model buffers once for each scene since we load this into vram once
    createModelBuffers();
    allocateLightingBuffers();
    createDescriptors(currentScene->textures, {});
    createShadowDescriptorSets();
    uniformSync = new DescriptorSyncer<UniformBufferObject>(this, {{ &mainPass.descriptorSets, 0 }, { &shadow.descriptorSets, 0 }}, 1);
    allocateCommandBuffers();
    initSynchronization();
    currentFrame = 0;
    double x, y;
    glfwSetCursor(window, cursors[CursorIndex::CURSOR_DEFAULT]);
    glfwGetCursorPos(window, &x, &y);
    lastMousePosition.x = x;
    lastMousePosition.y = y;
    auto mouseNormed = normedDevice(x, y);
    lastMousePosition.normedX = mouseNormed.first;
    lastMousePosition.normedY = mouseNormed.second;

    cammera.position = glm::vec3({ 3.0f, 3.0f, 3.0f });
    cammera.target = glm::vec3({ 0.0f, 0.0f, 0.0f });

    GuiTextures::setDefaultTexture();
    writeHudDescriptors();
    gui = new Gui(&lastMousePosition.normedX, &lastMousePosition.normedY, swapChainExtent.height, swapChainExtent.width);

    // we need to get stuff for the first frame on the device
    currentScene->updateUniforms(0);
    hudVertexCount = gui->updateBuffer(hudAllocation->GetMappedData(), 50);
    lightingDirty.resize(concurrentFrames);
    fill(lightingDirty.begin(), lightingDirty.end(), true);
    updateScene(0);
    vkDeviceWaitIdle(device);

    while (!glfwWindowShouldClose(window)) {
        drawFrame();
        glfwPollEvents();
        handleInput();
    }

    vkDeviceWaitIdle(device);
}

void Engine::loadDefaultScene() {
    //currentScene = new Scene(this, {{"models/viking_room.obj", "textures/viking_room.png"}}, 50);
    // currentScene = new Scene(this, {{"models/spaceship.obj", "textures/spaceship.png"}}, 50);
    currentScene = new Scene(this, {
        {"models/spaceship.obj", "textures/spaceship.png", "textures/spaceship_icon.png"},
        {"models/viking_room.obj", "textures/viking_room.png", ""},
        {"models/sphere.obj", "textures/sphere.png", ""}
    }, 50, {"skyboxes/front.png", "skyboxes/back.png", "skyboxes/up.png", "skyboxes/down.png", "skyboxes/right.png", "skyboxes/left.png"});
    currentScene->makeBuffers();
    // This first is the skybox (the position and heading do not matter)
    currentScene->addInstance(2, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f, 0.0f});
    currentScene->addInstance(0, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f, 0.0f});
    // currentScene->addInstance(1, { 5.0f, 0.0f, 0.0f}, { 0.0f, 0.0f, 0.0f});
    // currentScene->addInstance(0, { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 0.0f});

    currentScene->panels.insert({ "hud", Panel("panels/hud.yaml") });
}

// TODO this function has a crappy name that confuses me
void Engine::createDescriptors(const std::vector<InternalTexture>& textures, const std::vector<GuiTexture>& hudTextures) {
    std::array<VkDescriptorPoolSize, 5> poolSizes {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[0].descriptorCount = concurrentFrames;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 4 * concurrentFrames;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = concurrentFrames;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[3].descriptorCount = concurrentFrames;
    poolSizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[4].descriptorCount = concurrentFrames;

    std::array<VkDescriptorPoolSize, 3> hudPoolSizes;
    hudPoolSizes[0].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    hudPoolSizes[0].descriptorCount = concurrentFrames;
    hudPoolSizes[1].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    hudPoolSizes[1].descriptorCount = concurrentFrames;
    hudPoolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    hudPoolSizes[2].descriptorCount = engineSettings.maxHudTextures * concurrentFrames;

    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = concurrentFrames;

    VkDescriptorPoolCreateInfo hudPoolInfo {};
    hudPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    hudPoolInfo.poolSizeCount = hudPoolSizes.size();
    hudPoolInfo.pPoolSizes = hudPoolSizes.data();
    hudPoolInfo.maxSets = concurrentFrames;

    vulkanErrorGuard(vkCreateDescriptorPool(device, &poolInfo, nullptr, &mainPass.descriptorPool), "Failed to create descriptor pool.");
    vulkanErrorGuard(vkCreateDescriptorPool(device, &hudPoolInfo, nullptr, &hudDescriptorPool), "Failed to create hud descriptor pool.");

    std::vector<VkDescriptorSetLayout> layouts(concurrentFrames, mainPass.descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = mainPass.descriptorPool;
    allocInfo.descriptorSetCount = concurrentFrames;
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSetLayout> hudLayouts(concurrentFrames, hudDescriptorLayout);
    VkDescriptorSetAllocateInfo hudAllocInfo {};
    hudAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    hudAllocInfo.descriptorPool = hudDescriptorPool;
    hudAllocInfo.descriptorSetCount = concurrentFrames;
    hudAllocInfo.pSetLayouts = hudLayouts.data();

    mainPass.descriptorSets.resize(concurrentFrames);
    hudDescriptorSets.resize(concurrentFrames);

    vulkanErrorGuard(vkAllocateDescriptorSets(device, &allocInfo, mainPass.descriptorSets.data()), "Failed to allocate descriptor sets.");
    vulkanErrorGuard(vkAllocateDescriptorSets(device, &hudAllocInfo, hudDescriptorSets.data()), "Failed to allocate hud descriptor sets.");

    for (size_t i = 0; i < concurrentFrames; i++) {
        // VkDescriptorBufferInfo bufferInfo {};
        // bufferInfo.buffer = uniformBuffers[i];
        // bufferInfo.offset = 0;
        // // Idk if this involves a performance hit other than always copying the whole buffer even if it is not completely filled (this should be very small)
        // // It does save updating the descriptor set though, which means it could be faster this way (this would be my guess)
        // bufferInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo lightingBufferInfo {};
        lightingBufferInfo.buffer = lightingBuffers[i];
        lightingBufferInfo.offset = 0;
        lightingBufferInfo.range = VK_WHOLE_SIZE;

        std::array<VkWriteDescriptorSet, 4> descriptorWrites {};

        // descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // descriptorWrites[0].dstSet = mainPass.descriptorSets[i];
        // descriptorWrites[0].dstBinding = 0;
        // descriptorWrites[0].dstArrayElement = 0;
        // descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        // descriptorWrites[0].descriptorCount = 1;
        // descriptorWrites[0].pBufferInfo = &bufferInfo;

        std::vector<VkDescriptorImageInfo> imageInfos(textures.size());
        for (int i = 0; i < textures.size(); i++) {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = textures[i].textureImageView;
            imageInfos[i].sampler = textures[i].textureSampler;
        }

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = mainPass.descriptorSets[i];
        descriptorWrites[0].dstBinding = 1;
        // Hmm, can we use this for arraying our textures in a smarter way?
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = imageInfos.size();
        descriptorWrites[0].pImageInfo = imageInfos.data();

        VkDescriptorImageInfo shadowImageInfo = {};
        shadowImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shadowImageInfo.imageView = shadow.imageView;
        shadowImageInfo.sampler = shadow.sampler;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = mainPass.descriptorSets[i];
        descriptorWrites[1].dstBinding = 2;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &shadowImageInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = mainPass.descriptorSets[i];
        descriptorWrites[2].dstBinding = 3;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &lightingBufferInfo;

        VkDescriptorImageInfo skyboxInfo;
        skyboxInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        skyboxInfo.imageView = currentScene->skybox.textureImageView;
        skyboxInfo.sampler = currentScene->skybox.textureSampler;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = mainPass.descriptorSets[i];
        descriptorWrites[3].dstBinding = 4;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pImageInfo = &skyboxInfo;

        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}

namespace GuiTextures {
    static GuiTexture *defaultTexture;
};

void Engine::writeHudDescriptors() {
    for (size_t i = 0; i < concurrentFrames; i++) {
        std::array<VkWriteDescriptorSet, 3> hudDescriptorWrites {};

        // I think maybe you can array input attachments
        VkDescriptorImageInfo descriptorImageInfo {};
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.imageView = subpassImageViews[i];
        descriptorImageInfo.sampler = VK_NULL_HANDLE;
        
        hudDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        hudDescriptorWrites[0].dstSet = hudDescriptorSets[i];
        hudDescriptorWrites[0].dstBinding = 0;
        hudDescriptorWrites[0].dstArrayElement = 0;
        hudDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        hudDescriptorWrites[0].descriptorCount = 1;
        hudDescriptorWrites[0].pImageInfo = &descriptorImageInfo;

        VkDescriptorImageInfo iconDescriptorImageInfo {};
        iconDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        iconDescriptorImageInfo.imageView = bgSubpassImageViews[i];
        iconDescriptorImageInfo.sampler = VK_NULL_HANDLE;

        hudDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        hudDescriptorWrites[1].dstSet = hudDescriptorSets[i];
        hudDescriptorWrites[1].dstBinding = 1;
        hudDescriptorWrites[1].dstArrayElement = 0;
        hudDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        hudDescriptorWrites[1].descriptorCount = 1;
        hudDescriptorWrites[1].pImageInfo = &iconDescriptorImageInfo;

        std::vector<VkDescriptorImageInfo> hudTextureInfos(engineSettings.maxHudTextures);
        for (int i = 0; i < engineSettings.maxHudTextures; i++) {
            hudTextureInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            hudTextureInfos[i].imageView = GuiTextures::defaultTexture->textureImageView;
            hudTextureInfos[i].sampler = GuiTextures::defaultTexture->textureSampler;
        }

        hudDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        hudDescriptorWrites[2].dstSet = hudDescriptorSets[i];
        hudDescriptorWrites[2].dstBinding = 2;
        hudDescriptorWrites[2].dstArrayElement = 0;
        hudDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        hudDescriptorWrites[2].descriptorCount = hudTextureInfos.size();
        hudDescriptorWrites[2].pImageInfo = hudTextureInfos.data();

        // vkUpdateDescriptorSets(device, hudDescriptorWrites.size(), hudDescriptorWrites.data(), 0, nullptr);
        vkUpdateDescriptorSets(device, hudDescriptorWrites.size(), hudDescriptorWrites.data(), 0, nullptr);
    }
}

// This is a hack to keep the refcount from hiting 0 on the textures in use by the gpu still, but with no guicomponents using it anymore
static std::vector<GuiTexture> textureRefs;

void Engine::rewriteHudDescriptors(const std::vector<GuiTexture *>& hudTextures) {
    size_t oldSize = textureRefs.size();
    if (hudTextures.empty() && !oldSize) return; // We got nothing to do here
    textureRefs.clear();

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        std::array<VkWriteDescriptorSet, 1> hudDescriptorWrites {};

        std::vector<VkDescriptorImageInfo> hudTextureInfos(std::max(hudTextures.size(), oldSize));
        int j;

        for (j = 0; j < hudTextures.size(); j++) {
            hudTextureInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            hudTextureInfos[j].imageView = hudTextures[j]->textureImageView;
            hudTextureInfos[j].sampler = hudTextures[j]->textureSampler;
            // hudTextureInfos[i].imageView = currentScene->state.instances[0].texture->textureImageView;
            // hudTextureInfos[i].sampler = currentScene->state.instances[0].texture->textureSampler;
        }
        for (; j < oldSize; j++) {
            hudTextureInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            hudTextureInfos[j].imageView = GuiTextures::defaultTexture->textureImageView;
            hudTextureInfos[j].sampler = GuiTextures::defaultTexture->textureSampler;
        }

        hudDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        hudDescriptorWrites[0].dstSet = hudDescriptorSets[i];
        hudDescriptorWrites[0].dstBinding = 2;
        hudDescriptorWrites[0].dstArrayElement = 0;
        hudDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        hudDescriptorWrites[0].descriptorCount = hudTextureInfos.size();
        hudDescriptorWrites[0].pImageInfo = hudTextureInfos.data();

        vkUpdateDescriptorSets(device, hudDescriptorWrites.size(), hudDescriptorWrites.data(), 0, nullptr);
    }

    // Now we leverage the reference counting to keep the textures from disapearing on us
    // textureRefs.reserve(hudTextures.size());
    // transform(hudTextures.begin(), hudTextures.end(), refs.begin(), [](GuiTexture *x)-> GuiTexture { return GuiTexture(*x); } );
    for(const auto textPtr : hudTextures) {
        textureRefs.push_back(GuiTexture(*textPtr));
    }
}

void Engine::updateLightingDescriptors(int index, const ViewProjPosNearFar& data) {
    memcpy(lightingBufferAllocations[index]->GetMappedData(), &data, sizeof(ViewProjPosNearFar));
}

void Engine::allocateCommandBuffers() {
    // We called setup code incorrectly if these are not equal
    assert(concurrentFrames == swapChainFramebuffers.size());
    commandBuffers.resize(concurrentFrames);
    transferCommandBuffers.resize(concurrentFrames);

    VkCommandBufferAllocateInfo commandAlocInfo {};
    commandAlocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandAlocInfo.commandPool = commandPool;
    commandAlocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandAlocInfo.commandBufferCount = concurrentFrames;

    vulkanErrorGuard(vkAllocateCommandBuffers(device, &commandAlocInfo, commandBuffers.data()), "Failed to allocate command buffers.");

    if (engineSettings.useConcurrentTransferQueue) {
        VkCommandBufferAllocateInfo transferCommandAlocInfo {};
        transferCommandAlocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        transferCommandAlocInfo.commandPool = transferCommandPool;
        transferCommandAlocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        transferCommandAlocInfo.commandBufferCount = concurrentFrames;
        vulkanErrorGuard(vkAllocateCommandBuffers(device, &transferCommandAlocInfo, transferCommandBuffers.data()), "Failed to allocate transfer command buffers.");
    }
}

#include "shaders/render_modes.h"

// I am sorry what I am sorry that all the main pass rendering looks like it was made by a monkey, I didn't know how to plan it out at the start
// cause I didn't know what I was ultimately going to need (I should refactor it and make it less idiotic, but I don't really feel like doing that rn)
void Engine::recordCommandBuffer(const VkCommandBuffer& buffer, const VkFramebuffer& framebuffer, const VkDescriptorSet& descriptorSet, 
    const VkDescriptorSet& hudDescriptorSet, const VkBuffer& hudBuffer, int index) {
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    vulkanErrorGuard(vkBeginCommandBuffer(buffer, &beginInfo), "Failed to begin recording command buffer.");

    runShadowPass(buffer, index);

    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 4> clearValues = {};
    clearValues[0].color = {{ 0.0f, 0.0f, 0.0f, 0.0f }};
    clearValues[1].depthStencil = { 1.0f, 0 };
    clearValues[2].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    clearValues[3].color = {{ 0.0f, 0.0f, 0.0f, 0.0f }};

    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[GP_WORLD]);

    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    uint32_t dynamicOffset = 0;

    vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // This loop indexing will change once the instance allocator changes
    for(int j = 1; j < uniformSync->validCount[index]; j++) {
        dynamicOffset = j * uniformSync->uniformSkip;
        if (currentScene->state.instances[j].renderAsIcon) {
            const auto ent = &currentScene->entities[currentScene->state.instances[j].entityIndex];
            vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 1, &dynamicOffset);
            pushConstants.renderType = RINT_ICON | (int)currentScene->state.instances[j].highlight * RFLAG_HIGHLIGHT;
            pushConstants.textureIndex = ent->hasIcon ? ent->iconIndex : currentScene->textures.size() - 1;
            vkCmdPushConstants(buffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);
            // The last model is has the icon buffer stuff (it is setup like this is the Scene constructor)
            vkCmdDrawIndexed(buffer, (currentScene->models.end() - 1)->indexCount, 1, (currentScene->models.end() - 1)->indexOffset, 0, 0);
            continue;
        }
        pushConstants.renderType = RINT_OBJ | (int)currentScene->state.instances[j].highlight * RFLAG_HIGHLIGHT;
        vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 1, &dynamicOffset);
        // 0th instance is always the skybox
        pushConstants.textureIndex = currentScene->entities[currentScene->state.instances[j].entityIndex].textureIndex;
        vkCmdPushConstants(buffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);
        // TODO we can bundle draw commands the entities are the same (this includes the textures)
        vkCmdDrawIndexed(buffer, (currentScene->state.instances.data() + j)->sceneModelInfo->indexCount, 1,
            (currentScene->state.instances.data() + j)->sceneModelInfo->indexOffset, 0, 0);
    }

    // Render the icons back to front
    // for(const int j : zSortedIcons) {
    //     dynamicOffset = j * uniformSkip;
    // }

    vkCmdNextSubpass(buffer, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[GP_BG]);

    vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // render the skybox
    dynamicOffset = 0;
    pushConstants.renderType = RINT_SKYBOX;
    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 1, &dynamicOffset);
    vkCmdPushConstants(buffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);
    vkCmdDrawIndexed(buffer, currentScene->state.instances.data()->sceneModelInfo->indexCount, 1,
            currentScene->state.instances.data()->sceneModelInfo->indexOffset, 0, 0);

    vkCmdNextSubpass(buffer, VK_SUBPASS_CONTENTS_INLINE);

    VkClearAttachment depthClear;
    depthClear.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthClear.colorAttachment = 0;
    depthClear.clearValue.depthStencil = { 1.0f, 0 };

    VkClearRect depthClearRect;
    depthClearRect.rect.offset = { 0, 0 };
    depthClearRect.rect.extent = swapChainExtent;
    depthClearRect.baseArrayLayer = 0;
    depthClearRect.layerCount = 1;

    vkCmdClearAttachments(buffer, 1, &depthClear, 1, &depthClearRect);

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[GP_HUD]);
    vkCmdBindVertexBuffers(buffer, 0, 1, &hudBuffer, offsets);
    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hudPipelineLayout, 0, 1, &hudDescriptorSet, 0, 0);

    // gui->lockPushConstant();
    // Why need fragment stage access here???? (Obviously cause I declared the layout to need it, but if I don't the validation layer complains)?????
    vkCmdPushConstants(buffer, hudPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GuiPushConstant), gui->pushConstant());
    // gui->unlockPushConstant();

    vkCmdDraw(buffer, guiOutOfDate ? Gui::dummyVertexCount : hudVertexCount, 1, 0, 0);

    vkCmdEndRenderPass(buffer);

    if (depthDump.makeDump)
        dumpDepthBuffer(buffer, index);

    vulkanErrorGuard(vkEndCommandBuffer(buffer), "Failed to record command buffer.");
}

void Engine::initSynchronization() {
    imageAvailableSemaphores.resize(concurrentFrames);
    renderFinishedSemaphores.resize(concurrentFrames);
    inFlightFences.resize(concurrentFrames);
    // imagesInFlight.resize(concurrentFrames, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(int i = 0; i < concurrentFrames; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create synchronization primatives.");
        // if (engineSettings.verbose) std::cout << std::hex << "Fence " << i << " : " << inFlightFences[i] << std::endl;
    }
}

void Engine::recreateSwapChain() {
    std::cout << "Recreating swapchain with thread: " << gettid() << std::endl;

    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipelines();
    createDepthResources();
    createFramebuffers();
    createDescriptors(currentScene->textures, {});
    writeHudDescriptors();
    // allocateCommandBuffers();
}

void Engine::cleanupSwapChain() {
    destroyDepthResources();

    for (auto framebuffer : swapChainFramebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    // vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());
    // if (engineSettings.useConcurrentTransferQueue)
    //     vkFreeCommandBuffers(device, transferCommandPool, transferCommandBuffers.size(), transferCommandBuffers.data());

    for (auto pipeline : graphicsPipelines)
        vkDestroyPipeline(device, pipeline, nullptr);
    // TODO I think we could technically keep the pipeline layouts even on resizing
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyPipelineLayout(device, linePipelineLayout, nullptr);
    vkDestroyPipelineLayout(device, hudPipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    destroyImageViews();

    vkDestroySwapchainKHR(device, swapChain, nullptr);

    vkDestroyDescriptorPool(device, mainPass.descriptorPool, nullptr);
    vkDestroyDescriptorPool(device, hudDescriptorPool, nullptr);
}

void Engine::cleanup() {
    cleanupSwapChain();

    destroyLightingBuffers();
    delete uniformSync;

    vkDestroyDescriptorSetLayout(device, mainPass.descriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, mainPass.lineDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, hudDescriptorLayout, nullptr);

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    for (size_t i = 0; i < concurrentFrames; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vmaDestroyBuffer(memoryAllocator, hudBuffer, hudAllocation);

    if (engineSettings.useConcurrentTransferQueue) vkDestroyCommandPool(device, transferCommandPool, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyCommandPool(device, guiCommandPool, nullptr);

    // vkDestroyPipelineCache(device, pipelineCache, nullptr);
    destroyShadowResources();
    vmaDestroyAllocator(memoryAllocator);

    vkDestroyDevice(device, nullptr);

    if (engineSettings.validationLayers.size()) DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    destroyCursors();
    glfwTerminate();
}

Engine::~Engine() {
    // Lazy badness
    if (std::uncaught_exceptions()) return;
    textureRefs.clear();
    delete currentScene;
    // there is state tracking associated with knowing what textures are being used, This makes it so none are and they are freed
    delete GuiTextures::defaultTexture;
    delete gui;
    cleanup();
}

void Engine::createShadowResources(bool createDebugging) {
    auto depthFormat = findDepthFormat();
    shadow.debugging = createDebugging;
    shadow.makeSnapshot = false;
    shadow.debugWritePending = false;

    VkAttachmentDescription attachmentDescription {};
    attachmentDescription.format = depthFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 0;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthReference;
    subpass.pResolveAttachments = nullptr;

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 1> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &attachmentDescription;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = dependencies.size();
    renderPassCreateInfo.pDependencies = dependencies.data();

    vulkanErrorGuard(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &shadow.renderPass), "Unable to create shadow render pass");

    auto vertShaderCode = readFile("shaders/shadow_vert.spv");
    
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    // VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(shadow.width);
    viewport.height = static_cast<float>(shadow.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 0.5f;

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = { shadow.width, shadow.height };

    VkPipelineViewportStateCreateInfo viewportState {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasEnable = VK_TRUE;
    // rasterizer.depthBiasConstantFactor = 9.25f;
    // rasterizer.depthBiasSlopeFactor = 3.75f;

    VkPipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // Idk if this is set right or if it matters
    VkPipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 0;
    colorBlending.pAttachments = nullptr;

    VkDescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    vulkanErrorGuard(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &shadow.descriptorLayout), "Failed to create shadow descriptor set layout.");

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &shadow.descriptorLayout;

    VkPushConstantRange pushConstantRange;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ShadowPushConstansts);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    vulkanErrorGuard(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &shadow.pipelineLayout), "Failed to create shadow pipeline layout.");

    VkPipelineDepthStencilStateCreateInfo depthStencil {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    std::array<VkDynamicState, 1> dynamicStateEnables = { VK_DYNAMIC_STATE_DEPTH_BIAS };
    VkPipelineDynamicStateCreateInfo dynamicStates = {};
    dynamicStates.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStates.dynamicStateCount = dynamicStateEnables.size();
    dynamicStates.pDynamicStates = dynamicStateEnables.data();
    dynamicStates.flags = 0;

    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 1;
    pipelineInfo.pStages = &vertShaderStageInfo;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStates;

    pipelineInfo.layout = shadow.pipelineLayout;
    pipelineInfo.renderPass = shadow.renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    pipelineInfo.pDepthStencilState = &depthStencil;

    // vulkanErrorGuard(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &shadow.pipeline), "Failed to create shadow graphics pipeline.");
    vulkanErrorGuard(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shadow.pipeline), "Failed to create shadow graphics pipeline.");

    vkDestroyShaderModule(device, vertShaderModule, nullptr);

    // For shadow mapping we only need a depth attachment
    VkImageCreateInfo imageInfo {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = shadow.width;
    imageInfo.extent.height = shadow.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(memoryAllocator, &imageInfo, &imageAllocCreateInfo, &shadow.image, &shadow.allocation, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Unable to create shadow framebuffer image");

    VkImageViewCreateInfo depthStencilView = {};
    depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthStencilView.format = depthFormat;
    depthStencilView.subresourceRange = {};
    depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthStencilView.subresourceRange.baseMipLevel = 0;
    depthStencilView.subresourceRange.levelCount = 1;
    depthStencilView.subresourceRange.baseArrayLayer = 0;
    depthStencilView.subresourceRange.layerCount = 1;
    depthStencilView.image = shadow.image;
    vulkanErrorGuard(vkCreateImageView(device, &depthStencilView, nullptr, &shadow.imageView), "Unable to create shadow framebuffer image view.");

    // Create sampler to sample from to depth attachment
    VkFilter shadowmapFilter = formatIsFilterable(physicalDevice, depthFormat, VK_IMAGE_TILING_OPTIMAL) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    VkSamplerCreateInfo sampler = {};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = shadowmapFilter;
    sampler.minFilter = shadowmapFilter;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.unnormalizedCoordinates = false;
    sampler.anisotropyEnable = false;
    sampler.maxAnisotropy = 1.0f;
    sampler.mipLodBias = 0.0f;
    sampler.minLod = 0.0f;
    sampler.maxLod = 100.0f;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    vulkanErrorGuard(vkCreateSampler(device, &sampler, nullptr, &shadow.sampler), "Unable to create shadow sampler.");

    // Create frame buffer
    VkFramebufferCreateInfo fbufCreateInfo = {};
    fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbufCreateInfo.renderPass = shadow.renderPass;
    fbufCreateInfo.attachmentCount = 1;
    fbufCreateInfo.pAttachments = &shadow.imageView;
    fbufCreateInfo.width = shadow.width;
    fbufCreateInfo.height = shadow.height;
    fbufCreateInfo.layers = 1;

    vulkanErrorGuard(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &shadow.framebuffer), "Unable to create shadow framebuffer.");

    if (shadow.debugging) {
        // TODO This code assumes that we are only using VK_FORMAT_D32_SFLOAT for the depth format

        VkBufferCreateInfo debuggingBufferInfo = {};
        debuggingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        debuggingBufferInfo.size = shadow.width * shadow.height * 4;
        debuggingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo debuggingAllocationInfo = {};
        debuggingAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
        debuggingAllocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo debuggingBufferAl = {};

        if (vmaCreateBuffer(memoryAllocator, &debuggingBufferInfo, &debuggingAllocationInfo, &shadow.debugBuffer, &shadow.debugAllocation, &shadow.debugAllocInfo) != VK_SUCCESS)
            throw std::runtime_error("Unable to allocate memory for shadow debug buffer.");
    }
}

void Engine::destroyShadowResources() {
    vkDestroyRenderPass(device, shadow.renderPass, nullptr);
    vkDestroyPipelineLayout(device, shadow.pipelineLayout, nullptr);
    vkDestroyPipeline(device, shadow.pipeline, nullptr);

    vkDestroyFramebuffer(device, shadow.framebuffer, nullptr);
    vkDestroySampler(device, shadow.sampler, nullptr);
    vkDestroyImageView(device, shadow.imageView, nullptr);
    vmaDestroyImage(memoryAllocator, shadow.image, shadow.allocation);

    if(shadow.debugging)
        vmaDestroyBuffer(memoryAllocator, shadow.debugBuffer, shadow.debugAllocation);

    vkDestroyDescriptorPool(device, shadow.descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, shadow.descriptorLayout, nullptr);
}

void Engine::writeShadowBufferToFile(const VkCommandBuffer& buffer, const char *filename) {
    assert(!shadow.debugWritePending);

    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = shadow.image;

    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // not sure about src mask ????
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    // I do know that the dst access mask is right
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    // also no clue about stage masks??? I am continually confused by these things
    vkCmdPipelineBarrier(
        buffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = shadow.width;
    region.imageExtent.height = shadow.height;
    region.imageExtent.depth = 1;

    vkCmdCopyImageToBuffer(buffer, shadow.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, shadow.debugBuffer, 1, &region);

    shadow.debugWritePending = true;
    shadow.filename = std::string(filename);

    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(
        buffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

// This can go away, but for right now it is convinient for checking things
static int histogram[256];

void Engine::doShadowDebugWrite() {
    memset(histogram, 0, 256 * sizeof(int));

    // idk about single channle png images?
    unsigned char *buf = new unsigned char[shadow.width * shadow.height];
    float *data = static_cast<float *>(shadow.debugAllocation->GetMappedData());

    for (int y = 0; y < shadow.height; y++) {
        for (int x = 0; x < shadow.width; x++) {
            unsigned char value = static_cast<unsigned char>(data[y * shadow.width + x] * 255.0f);
            buf[y * shadow.width + x] = value;
            histogram[value] += 1;
        }
    }

    stbi_write_png(shadow.filename.c_str(), shadow.width, shadow.height, 1, buf, shadow.width);

    delete buf;

    shadow.debugWritePending = false;
}

void Engine::createShadowDescriptorSets() {
    std::array<VkDescriptorPoolSize, 1> poolSizes {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[0].descriptorCount = concurrentFrames;

    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = concurrentFrames;

    vulkanErrorGuard(vkCreateDescriptorPool(device, &poolInfo, nullptr, &shadow.descriptorPool), "Failed to create shadow descriptor pool.");

    std::vector<VkDescriptorSetLayout> layouts(concurrentFrames, shadow.descriptorLayout);
    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = shadow.descriptorPool;
    allocInfo.descriptorSetCount = concurrentFrames;
    allocInfo.pSetLayouts = layouts.data();

    shadow.descriptorSets.resize(concurrentFrames);

    vulkanErrorGuard(vkAllocateDescriptorSets(device, &allocInfo, shadow.descriptorSets.data()), "Failed to allocate shadow descriptor sets.");

    // for (size_t i = 0; i < concurrentFrames; i++) {
    //     VkDescriptorBufferInfo bufferInfo {};
    //     bufferInfo.buffer = uniformBuffers[i];
    //     bufferInfo.offset = 0;
    //     bufferInfo.range = VK_WHOLE_SIZE;

    //     std::array<VkWriteDescriptorSet, 1> descriptorWrites {};

    //     descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //     descriptorWrites[0].dstSet = shadow.descriptorSets[i];
    //     descriptorWrites[0].dstBinding = 0;
    //     descriptorWrites[0].dstArrayElement = 0;
    //     descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    //     descriptorWrites[0].descriptorCount = 1;
    //     descriptorWrites[0].pBufferInfo = &bufferInfo;

    //     vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    // }
}

void Engine::runShadowPass(const VkCommandBuffer& buffer, int index) {
    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = shadow.renderPass;
    renderPassInfo.framebuffer = shadow.framebuffer;

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { shadow.width, shadow.height };

    std::array<VkClearValue, 1> clearValues = {};
    clearValues[0].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetDepthBias(buffer, 2.0f, 0.0f, 1.5f);

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow.pipeline);

    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // This loop indexing will change once the instance allocator changes
    for(int j = 1; j < uniformSync->validCount[index]; j++) {
        uint32_t dynamicOffset = j * uniformSync->uniformSkip;
        vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow.pipelineLayout, 0, 1, &shadow.descriptorSets[index], 1, &dynamicOffset);
        vkCmdPushConstants(buffer, shadow.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowPushConstansts), &shadow.constants);
        // TODO we can bundle draw commands the entities are the same (this includes the textures)
        vkCmdDrawIndexed(buffer, (currentScene->state.instances.data() + j)->sceneModelInfo->indexCount, 1,
            (currentScene->state.instances.data() + j)->sceneModelInfo->indexOffset, 0, 0);
    }

    vkCmdEndRenderPass(buffer);

    if (shadow.makeSnapshot && shadow.debugging) {
        writeShadowBufferToFile(buffer, shadowMapFile);
        shadow.makeSnapshot = false;
    }
}

// Just check for linear filtering support
VkBool32 Engine::formatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling)
{
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);

    if (tiling == VK_IMAGE_TILING_OPTIMAL)
        return formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

    if (tiling == VK_IMAGE_TILING_LINEAR)
        return formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

    return false;
}

void Engine::createCursors() {
    GLFWimage imageData;
    int channels;
    for(const auto& data : cursorData) {
        imageData.pixels = stbi_load(get<0>(data), &imageData.width, &imageData.height, &channels, STBI_rgb_alpha);

        auto ret = glfwCreateCursor(&imageData, get<1>(data), get<2>(data));
        if (!ret) throw std::runtime_error("Unable to create cursor.");
        cursors.push_back(ret);

        stbi_image_free(imageData.pixels);
    }
}

void Engine::destroyCursors() {
    for(auto& cursor : cursors)
        glfwDestroyCursor(cursor);
}

namespace InternalTextures {
    static std::map<VkImage, uint> references = {};
};

InternalTexture::InternalTexture(Engine *context, TextureCreationData creationData) {
    this->context = context;
    width = creationData.width;
    height = creationData.height;
    channels = creationData.channels;
    VkDeviceSize imageSize = width * height * creationData.channels;

    VkFormat format = Utilities::channelsToFormat(channels);

    VkBufferCreateInfo stagingBufInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    stagingBufInfo.size = imageSize;
    stagingBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingBufAllocCreateInfo = {};
    stagingBufAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingBufAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo stagingBufferAllocationInfo = {};
    if (vmaCreateBuffer(context->memoryAllocator, &stagingBufInfo, &stagingBufAllocCreateInfo, &stagingBuffer, &stagingBufferAllocation, &stagingBufferAllocationInfo) !=VK_SUCCESS)
        throw std::runtime_error("Unable to allocate memory for texture.");

    memcpy(stagingBufferAllocationInfo.pMappedData, creationData.pixels, imageSize);

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(context->memoryAllocator, &imageInfo, &imageAllocCreateInfo, &textureImage, &textureAllocation, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Unable to create image");

    // Hm, how to use the transfer queue for this?
    // VkCommandBuffer commandBuffer = context->engineSettings.useConcurrentTransferQueue ?
    //     context->beginSingleTimeCommands(context->transferCommandPool) : context->beginSingleTimeCommands();
    VkCommandBuffer commandBuffer = context->beginSingleTimeCommands();

    VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.image = textureImage;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageMemoryBarrier.image = textureImage;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);

    context->endSingleTimeCommands(commandBuffer);

    vmaDestroyBuffer(context->memoryAllocator, stagingBuffer, stagingBufferAllocation);

    VkImageViewCreateInfo textureImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    textureImageViewInfo.image = textureImage;
    textureImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    textureImageViewInfo.format = format;
    textureImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // Fix this
    textureImageViewInfo.subresourceRange.baseMipLevel = 0;
    textureImageViewInfo.subresourceRange.levelCount = 1;
    textureImageViewInfo.subresourceRange.baseArrayLayer = 0;
    textureImageViewInfo.subresourceRange.layerCount = 1;
    vulkanErrorGuard(vkCreateImageView(context->device, &textureImageViewInfo, nullptr, &textureImageView), "Unable to create texture image view.");

    InternalTextures::references.insert({ textureImage, 1 });

    VkSamplerCreateInfo samplerInfo {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    // For right now we just use the max supported sampler anisotrpy
    samplerInfo.maxAnisotropy = context->maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    // fix me
    samplerInfo.maxLod = static_cast<float>(1);
    samplerInfo.mipLodBias = 0.0f;

    vulkanErrorGuard(vkCreateSampler(context->device, &samplerInfo, nullptr, &textureSampler), "Failed to create texture sampler.");
}

// MaxLoD is not working right cause something is wrong
void InternalTexture::generateMipmaps() {
    // I will leave this in here in case we want to handle multiple image formats in the future
    VkFormat imageFormat = Utilities::channelsToFormat(channels);
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(context->physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        throw std::runtime_error("Texture image format does not support linear blitting.");

    VkCommandBuffer commandBuffer = context->beginSingleTimeCommands();

    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = textureImage;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    for (uint32_t i = 1; i < mipLevels; i++) {
        // std::cout << std::dec << "Blitting mipmap level: " << i  << std::endl;

        const std::int32_t mipWidth = std::max<int32_t>(width >> i, 1);
        const std::int32_t mipHeight = std::max<int32_t>(height >> i, 1);

        VkImageBlit imageBlit = {};
        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.baseArrayLayer = 0;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel = 0;
        imageBlit.srcOffsets[ 0 ] = { 0, 0, 0 };
        imageBlit.srcOffsets[ 1 ] = { width, height, 1 };

        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.baseArrayLayer = 0;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstOffsets[ 0 ] = { 0, 0, 0 };
        imageBlit.dstOffsets[ 1 ] = { mipWidth, mipHeight, 1 };

        vkCmdBlitImage(commandBuffer,
            textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &imageBlit,
            VK_FILTER_LINEAR);

        barrier.subresourceRange.baseMipLevel = i;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
    }

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    context->endSingleTimeCommands(commandBuffer);
    // if (context->engineSettings.useConcurrentTransferQueue) context->endSingleTimeCommands(commandBuffer, context->transferCommandPool, context->transferQueue);
    // else context->endSingleTimeCommands(commandBuffer);
}

InternalTexture::~InternalTexture() {
    if (--InternalTextures::references[textureImage] == 0) {
        vkDestroySampler(context->device, textureSampler, nullptr);
        vkDestroyImageView(context->device, textureImageView, nullptr);
        vmaDestroyImage(context->memoryAllocator, textureImage, textureAllocation);
        InternalTextures::references.erase(textureImage);
    }
}

InternalTexture::InternalTexture(const InternalTexture& other) {
    textureImageView = other.textureImageView;
    textureImage = other.textureImage;
    textureSampler = other.textureSampler;
    textureAllocation = other.textureAllocation;
    context = other.context;
    mipLevels = other.mipLevels;
    width = other.width;
    height = other.height;
    InternalTextures::references[textureImage]++;
}

InternalTexture& InternalTexture::operator=(const InternalTexture& other) {
    return *this = InternalTexture(other);
}

InternalTexture::InternalTexture(InternalTexture&& other) noexcept
: textureImageView(other.textureImageView), textureSampler(other.textureSampler), mipLevels(other.mipLevels),
textureImage(other.textureImage), context(other.context), textureAllocation(other.textureAllocation),
width(other.width), height(other.height)
{ InternalTextures::references[textureImage]++; }

InternalTexture& InternalTexture::operator=(InternalTexture&& other) noexcept {
    textureImageView = other.textureImageView;
    textureImage = other.textureImage;
    textureSampler = other.textureSampler;
    textureAllocation = other.textureAllocation;
    context = other.context;
    mipLevels = other.mipLevels;
    width = other.width;
    height = other.height;
    return *this;
}

Scene::Scene(Engine* context, std::vector<std::tuple<const char *, const char *, const char *>> entities, size_t initialSize, std::array<const char *, 6> skyboxImages)
: skybox(context, skyboxImages), zSorter(instanceZSorter(this)) {
    this->context = context;
    Entity icon(ENT_ICON);
    for(const auto& entity : entities) {
        this->entities.push_back(Entity(get<0>(entity), get<1>(entity), get<2>(entity)));
    }
    for(auto& entity : this->entities) {
        if (entity.hasTexture) {
            entity.textureIndex = textures.size();
            textures.push_back(InternalTexture(context, { entity.textureHeight, entity.textureWidth, entity.textureChannels, entity.texturePixels }));
        }
        if (entity.hasIcon) {
            entity.iconIndex = textures.size();
            textures.push_back(InternalTexture(context, { entity.iconHeight, entity.iconWidth, entity.iconChannels, entity.iconPixels }));
        }
    }
    this->entities.push_back(icon);
    textures.push_back(InternalTexture(context, { icon.textureHeight, icon.textureWidth, icon.textureChannels, icon.texturePixels }));

    state.instances.reserve(initialSize);
}

void Scene::addInstance(int entityIndex, glm::vec3 position, glm::vec3 heading) {
    if (!models.size()) throw std::runtime_error("Please make the model buffers before adding instances.");
    state.instances.push_back(Instance(entities.data() + entityIndex, textures.data() + entityIndex, models.data() + entityIndex, entityIndex));
    state.instances.back().position = std::move(position);
    state.instances.back().heading = std::move(heading);
}

void Scene::makeBuffers() {
    size_t vertexOffset = 0, indicesOffset = 0;
    for(const auto& entity : entities) {
        models.push_back({ vertexOffset, indicesOffset, entity.indices.size() });
        vertexBuffer.insert(vertexBuffer.end(), entity.vertices.begin(), entity.vertices.end());
        for(const auto& idx : entity.indices) {
            indexBuffer.push_back(idx + vertexOffset);
        }
        vertexOffset += entity.vertices.size();
        indicesOffset += entity.indices.size();
    }
}

void Scene::updateUniforms(int idx) {
    auto view_1proj_1 = inverse(context->pushConstants.view) * inverse(context->pushConstants.projection);
    float aspectRatio = context->swapChainExtent.width / (float) context->swapChainExtent.height;
    context->uniformSync->sync(idx, state.instances.size(), [this, view_1proj_1, aspectRatio](size_t n) -> UniformBufferObject * {
        return this->state.instances[n].state(context->pushConstants.view, context->pushConstants.projection, view_1proj_1, aspectRatio,
            context->cammera.minClip, context->cammera.maxClip);
    });
    // for (int i = 0; i < state.instances.size(); i++) {
    //     memcpy(static_cast<unsigned char *>(buffer) + i * uniformSkip,
    //         (state.instances.data() + i)->state(context->pushConstants.view, context->pushConstants.projection, view_1proj_1, aspectRatio,
    //             context->cammera.minClip, context->cammera.maxClip), sizeof(UniformBufferObject));
    // }
}

namespace CubeMaps {
    static std::map<VkImage, uint> references = {};
};

CubeMap::CubeMap(Engine *context, std::array<const char *, 6> files) {
    this->context = context;
    std::vector<uint8_t *> pixelsBuffers;

    int height, width;
    bool set = false;
    int readHeight, readWidth, readChannles;
    for (const auto& filename : files) {
        pixelsBuffers.push_back(stbi_load(filename, &readHeight, &readWidth, &readChannles, STBI_rgb_alpha));
        if (readChannles != 4) throw std::runtime_error("Cubemap resource format incorrect.");
        // I know this is true for open gl shaders in glsl, idk about vulkan, but just to be safe for now
        if (readHeight != readWidth) throw std::runtime_error("Cubemap textures need to be square.");
        if (!set) {
            height = readHeight;
            width = readWidth;
        } else {
            if (readHeight != height || readWidth != width)
                throw std::runtime_error("Cubemap resource mismatching sizes.");
        }
    }

    VkBufferCreateInfo stagingBufInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    stagingBufInfo.size = height * width * 24;
    stagingBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingBufAllocCreateInfo = {};
    stagingBufAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingBufAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo stagingBufferAllocationInfo = {};
    if (vmaCreateBuffer(context->memoryAllocator, &stagingBufInfo, &stagingBufAllocCreateInfo, &stagingBuffer, &stagingBufferAllocation, &stagingBufferAllocationInfo) != VK_SUCCESS)
        throw std::runtime_error("Unable to allocate memory for texture.");

    for (int i = 0; i < 6; i++)
        memcpy((uint8_t *)stagingBufferAllocationInfo.pMappedData + height * width * 4 * i, pixelsBuffers[i], height * width * 4);

    for (const auto& buffer : pixelsBuffers)
        stbi_image_free(buffer);

    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D ;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(context->memoryAllocator, &imageInfo, &imageAllocCreateInfo, &textureImage, &textureAllocation, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Unable to create image");

    // Hm, how to use the transfer queue for this?
    // VkCommandBuffer commandBuffer = context->engineSettings.useConcurrentTransferQueue ?
    //     context->beginSingleTimeCommands(context->transferCommandPool) : context->beginSingleTimeCommands();
    VkCommandBuffer commandBuffer = context->beginSingleTimeCommands();

    VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 6;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.image = textureImage;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 6;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageMemoryBarrier.image = textureImage;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);

    context->endSingleTimeCommands(commandBuffer);

    vmaDestroyBuffer(context->memoryAllocator, stagingBuffer, stagingBufferAllocation);

    VkImageViewCreateInfo textureImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    textureImageViewInfo.image = textureImage;
    textureImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    textureImageViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    textureImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    textureImageViewInfo.subresourceRange.baseMipLevel = 0;
    textureImageViewInfo.subresourceRange.levelCount = 1;
    textureImageViewInfo.subresourceRange.baseArrayLayer = 0;
    textureImageViewInfo.subresourceRange.layerCount = 6;
    vulkanErrorGuard(vkCreateImageView(context->device, &textureImageViewInfo, nullptr, &textureImageView), "Unable to create texture image view.");

    CubeMaps::references.insert({ textureImage, 1 });

    VkSamplerCreateInfo samplerInfo {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    // For right now we just use the max supported sampler anisotrpy
    samplerInfo.maxAnisotropy = context->maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.mipLodBias = 0.0f;

    vulkanErrorGuard(vkCreateSampler(context->device, &samplerInfo, nullptr, &textureSampler), "Failed to create texture sampler.");
}

CubeMap::~CubeMap() {
    if (--CubeMaps::references[textureImage] == 0) {
        vkDestroySampler(context->device, textureSampler, nullptr);
        vkDestroyImageView(context->device, textureImageView, nullptr);
        vmaDestroyImage(context->memoryAllocator, textureImage, textureAllocation);
        CubeMaps::references.erase(textureImage);
    }
}

CubeMap::CubeMap(const CubeMap& other) {
    textureImageView = other.textureImageView;
    textureImage = other.textureImage;
    textureSampler = other.textureSampler;
    textureAllocation = other.textureAllocation;
    context = other.context;
    CubeMaps::references[textureImage]++;
}

CubeMap& CubeMap::operator=(const CubeMap& other) {
    return *this = CubeMap(other);
}

CubeMap::CubeMap(CubeMap&& other) noexcept
: textureImageView(other.textureImageView), textureSampler(other.textureSampler),
textureImage(other.textureImage), context(other.context), textureAllocation(other.textureAllocation)
{ CubeMaps::references[textureImage]++; }

CubeMap& CubeMap::operator=(CubeMap&& other) noexcept {
    textureImageView = other.textureImageView;
    textureImage = other.textureImage;
    textureSampler = other.textureSampler;
    textureAllocation = other.textureAllocation;
    context = other.context;
    return *this;
}

namespace GuiTextures {
    // I should have just used shared pointers from the start
    static std::map<VkImage, uint> references = {};
    static FT_Library library;
    static FT_Face face;
    static int maxGlyphHeight;
    static Engine *context;
};

GuiTexture *GuiTexture::defaultTexture() { return GuiTextures::defaultTexture; };

GuiTexture::GuiTexture(Engine *context, void *pixels, int width, int height, int channels, int strideBytes, VkFormat format, VkFilter filter) {
    this->context = context;
    widenessRatio = (float)width / height;

    VkBufferCreateInfo stagingBufInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    stagingBufInfo.size = height * width * channels;
    stagingBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingBufAllocCreateInfo = {};
    stagingBufAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingBufAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo stagingBufferAllocationInfo = {};
    if (vmaCreateBuffer(context->memoryAllocator, &stagingBufInfo, &stagingBufAllocCreateInfo, &stagingBuffer,
        &stagingBufferAllocation, &stagingBufferAllocationInfo) != VK_SUCCESS)
            throw std::runtime_error("Unable to allocate memory for texture.");

    for (int i = 0; i < height; i++)
        memcpy((uint8_t *)stagingBufferAllocationInfo.pMappedData + i * width, (uint8_t *)pixels + i * strideBytes, width * 4 * channels);

    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D ;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(context->memoryAllocator, &imageInfo, &imageAllocCreateInfo, &textureImage, &textureAllocation, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Unable to create image");

    VkCommandBuffer commandBuffer = context->beginSingleTimeCommands(context->guiCommandPool);

    VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.image = textureImage;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageMemoryBarrier.image = textureImage;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);

    context->endSingleTimeCommands(commandBuffer, context->guiCommandPool, context->graphicsQueue);

    vmaDestroyBuffer(context->memoryAllocator, stagingBuffer, stagingBufferAllocation);

    VkImageViewCreateInfo textureImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    textureImageViewInfo.image = textureImage;
    textureImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    textureImageViewInfo.format = format;
    textureImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    textureImageViewInfo.subresourceRange.baseMipLevel = 0;
    textureImageViewInfo.subresourceRange.levelCount = 1;
    textureImageViewInfo.subresourceRange.baseArrayLayer = 0;
    textureImageViewInfo.subresourceRange.layerCount = 1;
    vulkanErrorGuard(vkCreateImageView(context->device, &textureImageViewInfo, nullptr, &textureImageView), "Unable to create texture image view.");

    GuiTextures::references.insert({ textureImage, 1 });

    VkSamplerCreateInfo samplerInfo {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // samplerInfo.magFilter = VK_FILTER_CUBIC_IMG;
    // samplerInfo.minFilter = VK_FILTER_CUBIC_IMG;
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    // For right now we just use the max supported sampler anisotrpy
    samplerInfo.maxAnisotropy = context->maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.mipLodBias = 0.0f;

    vulkanErrorGuard(vkCreateSampler(context->device, &samplerInfo, nullptr, &textureSampler), "Failed to create texture sampler.");
}

GuiTexture::~GuiTexture() {
    if (--GuiTextures::references[textureImage] == 0) {
        vkDestroySampler(context->device, textureSampler, nullptr);
        vkDestroyImageView(context->device, textureImageView, nullptr);
        vmaDestroyImage(context->memoryAllocator, textureImage, textureAllocation);
        GuiTextures::references.erase(textureImage);
    }
}

GuiTexture::GuiTexture(const GuiTexture& other) {
    textureImageView = other.textureImageView;
    textureImage = other.textureImage;
    textureSampler = other.textureSampler;
    textureAllocation = other.textureAllocation;
    context = other.context;
    widenessRatio = other.widenessRatio;
    GuiTextures::references[textureImage]++;
}

GuiTexture& GuiTexture::operator=(const GuiTexture& other) {
    return *this = GuiTexture(other);
}

GuiTexture::GuiTexture(GuiTexture&& other) noexcept
: textureImageView(other.textureImageView), textureSampler(other.textureSampler), widenessRatio(other.widenessRatio),
textureImage(other.textureImage), context(other.context), textureAllocation(other.textureAllocation)
{ GuiTextures::references[textureImage]++; }

GuiTexture& GuiTexture::operator=(GuiTexture&& other) noexcept {
    textureImageView = other.textureImageView;
    textureImage = other.textureImage;
    textureSampler = other.textureSampler;
    textureAllocation = other.textureAllocation;
    context = other.context;
    widenessRatio = other.widenessRatio;
    return *this;
}

void GuiTexture::syncTexturesToGPU(const std::vector<GuiTexture *>& textures) {
    context->rewriteHudDescriptors(textures);
}

bool GuiTexture::operator==(const GuiTexture& other) const {
    return textureImage == other.textureImage;
}

ResourceID GuiTexture::resourceID() {
    return textureImage;
}

// This font stuff is ultrajank
// It may seem less than ideal that we render to a buffer like this, but indeed this is how freetype does things anyways, with this psuedo global state
namespace GuiTextures {
    static void *textTextureBuffer;
    void destroyTextTextureBuffer() {
        ::operator delete(textTextureBuffer);
    }
    const int maxTextWidth = 2048;
    
    // I know what you are thinking...
    //  ...that this should be an object since I practically gave it a destructor with the atexit bit
    void initFreetype2(Engine *context) {
        GuiTextures::context = context;
        FT_Error error = FT_Init_FreeType(&library);
        if (error) throw std::runtime_error("Unable to initialize freetype.");

        error = FT_New_Face(GuiTextures::library, "fonts/FreeSansBold.ttf", 0, &face);
        if (error == FT_Err_Unknown_File_Format) throw std::runtime_error("Unsupported font format.");
        else if (error) throw std::runtime_error("Unable to read font file.");
        
        // std::cout << "Font contains glyph count: " << face->num_glyphs << std::endl;

        error = FT_Set_Char_Size(face, 0, 12*64, 300, 300);
        if (error) throw std::runtime_error("Unable to setup font.");

        error = FT_Load_Glyph(face, FT_Get_Char_Index(face, 'T'), 0);
        if (error) throw std::runtime_error("Unable to load glyph.");
        error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_SDF);
        if (error) throw std::runtime_error("Unable to render glyph.");
        maxGlyphHeight = face->glyph->bitmap_top;

        textTextureBuffer = ::operator new(maxGlyphHeight * 2 * maxTextWidth);
        atexit(destroyTextTextureBuffer);
    }

    // This function seems really slow
    // !!! Multiline will work incorrectly !!!
    int makeTexture(const char *str) {
        FT_Vector pen;
        pen.x = maxGlyphHeight / 2;
        pen.y = maxGlyphHeight / 2;
        memset(textTextureBuffer, 0, maxGlyphHeight * 2 * maxTextWidth);
        const char *it = str;
        while(*it) {
            // Only for ascii rn
            FT_Error error = FT_Load_Glyph(face, FT_Get_Char_Index(face, *it), 0);
            if (error) throw std::runtime_error("Unable to load glyph.");
            error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_SDF);
            if (error) throw std::runtime_error("Unable to render glyph.");

            for(int i = 0; i < face->glyph->bitmap.rows; i++)
                for(int j = 0; j < abs(face->glyph->bitmap.pitch); j++)
                    // Ah, silent failures... the best kind (it should be obvious in context though)
                    if (face->glyph->bitmap_left + pen.x + j < maxTextWidth)
                        *((unsigned char *)textTextureBuffer + (pen.y + maxGlyphHeight - face->glyph->bitmap_top + i) * maxTextWidth + face->glyph->bitmap_left + pen.x + j) = 
                            std::max(*(face->glyph->bitmap.buffer + i * abs(face->glyph->bitmap.pitch) + j),
                                *((unsigned char *)textTextureBuffer + (pen.y + maxGlyphHeight - face->glyph->bitmap_top + i) * maxTextWidth + face->glyph->bitmap_left + pen.x + j));

            pen.x += face->glyph->advance.x >> 6;
            // pen.y += face->glyph->advance.y >> 6;

            it++;
        }

        // stbi_write_png("glyph.png", maxTextWidth, maxGlyphHeight * 2, 1, textTextureBuffer, maxTextWidth);

        return pen.x + maxGlyphHeight / 2;
    }

    GuiTexture makeGuiTexture(const char *str) {
        int width = makeTexture(str);

        // tbh I have no idea how to sharpen the text correctly
        return GuiTexture(context, textTextureBuffer, width, maxGlyphHeight * 2, 1, maxTextWidth, VK_FORMAT_R8_SRGB, VK_FILTER_LINEAR);
    }

    void setDefaultTexture() {
        unsigned char value[] = {
            0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
            0xa0, 0xff, 0xff, 0xff, 0xff, 0xa0,
            0xa0, 0xff, 0xff, 0xff, 0xff, 0xa0,
            0xa0, 0xff, 0xff, 0xff, 0xff, 0xa0,
            0xa0, 0xff, 0xff, 0xff, 0xff, 0xa0,
            0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
        };
        defaultTexture = new GuiTexture(context, &value, 6, 6, 1, 6, VK_FORMAT_R8_SRGB);
    }
};

template<typename T> void DescriptorSyncer<T>::sync(int descriptorIndex, size_t count, std::function<T *(size_t idx)> func) {
    if (count > currentSize[descriptorIndex]) {
        reallocateBuffer(descriptorIndex, count + 50);
    }
    // assume for right now that this updates every valid instance
    validCount[descriptorIndex] = count;
    for (size_t i = 0; i < count; i++) {
        memcpy(static_cast<unsigned char *>(uniformBufferAllocations[descriptorIndex]->GetMappedData()) + i * uniformSkip,
            func(i), sizeof(T));
    }
}