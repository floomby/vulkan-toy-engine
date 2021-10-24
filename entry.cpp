#include "engine.hpp"

#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>

namespace po = boost::program_options;

void run(EngineSettings& settings);

int main(int argc, char **argv) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Print help message")
        ("debug", "Enable extra debug options")
        ("use-one-queue", "Disable use of the transfer queue")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    EngineSettings settings = {};
    if (vm.count("use-one-queue"))
        settings.useConcurrentTransferQueue = false;
    else
        settings.useConcurrentTransferQueue = true;
    settings.height = 900;
    settings.width = 1600;
    settings.validationLayers = { "VK_LAYER_KHRONOS_validation" };
    // settings.maxFramesInFlight = 3;
    settings.applicationName = "Vulkan Test";
    settings.extremelyVerbose = false;
    settings.verbose = true;
    if (vm.count("debug")) {
        settings.validationExtentions = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
        settings.extremelyVerbose = true;
    } else {
        settings.validationExtentions = { };
    }
    settings.maxTextures = 1024;
    settings.maxHudTextures = 128;


    std::ofstream log;
    log.open("log");
    log.close();

    try {
        run(settings);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        log << e.what() << std::endl;
        log.close();    
        return EXIT_FAILURE;
    }

    log << "exited normally" << std::endl;
    log.close();
    return EXIT_SUCCESS;
}

// TODO I do horrible things by inspecting stack unwinding in destructors so I need raii pattern here before the catch
void run(EngineSettings& settings) {
    Engine engine(settings);
    engine.init();
    engine.runCurrentScene();
}

/*
Random non-crital stuff:
 I am linking to libpng now for freetype2, maybe I should stop using stbi
 The makefile build should be made faster (probably some code changes needed though to not have to recomplile so much temlate code every time)
 make concurrent frames correspond to the command buffers, descriptor sets and so forth while swapchain.size() is used for the framebuffer and other stuff like that

Next steps
 cpu (host) allocation stuff for vulkan (vma)?? (This is probably not worth doing until I know it is bad as it is)
 game instance allocation synchronization with vulkan buffers
 game state (this involves concurrency stuff)
 ECS? (or something else as an organizational structure for the proccessing)
 lt - better model library (assimp?)
 lt - emmisivity maps
 gui
  finish syncronization (I need to protect the verticies buffer, if the gui thread triggers a write while we are already flagged as rebuilt it can get messed up)
  messaging from the gui to the render thread (probably just use another spsc lockfree queue)
  textures for gui elements (this does the text too basically)
  yaml reader into layouts
  pass the clicks to the gui from the render thread (this is easy, just give it the ndc in the gui input message queue)
 culling??? (maybe just use https://gist.github.com/podgorskiy/e698d18879588ada9014768e3e82a644, but it does use aabb)
 pcf filtering for shadows
 dynamic backface culling or fix normals for skybox geometry
 nlips?
 explosions - particles?

Bugfixxy stuff
 get lod working
 resizing rarly, but sometimes does this:
    Validation layer: Validation Error: [ VUID-VkSwapchainCreateInfoKHR-imageExtent-01274 ] Object 0: handle = 0x560cc52946a8, type = VK_OBJECT_TYPE_DEVICE;
    | MessageID = 0x7cd0911d | vkCreateSwapchainKHR() called with imageExtent = (1326,610), which is outside the bounds returned by
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(): currentExtent = (1293,581), minImageExtent = (1293,581), maxImageExtent = (1293,581). The Vulkan spec
    states: imageExtent must be between minImageExtent and maxImageExtent, inclusive, where minImageExtent and maxImageExtent are members of the
    VkSurfaceCapabilitiesKHR structure returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR for the surface
    (https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VUID-VkSwapchainCreateInfoKHR-imageExtent-01274)

 get the api inspector figured out (I am crashing on vkEnumeratePhysicalDevices)
*/