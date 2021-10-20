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
    settings.height = 600;
    settings.width = 800;
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
Next steps
 cpu (host) allocation stuff for vulkan (vma)?? (This is probably not worth doing until I know it is bad as it is)
 game instance allocation synchronization with vulkan buffers
 game state (this involves concurrency stuff)
 ECS? (or something else as an organizational structure for the proccessing)
 lt - better model library (assimp?)
 lt - emmisivity maps
 makefile build faster (probably some code changes needed though to not have to recomplile so much temlate code every time)
 gui
  threading (think how to tell the gui object to do things)
  fonts -  I will be uising a library of some sort for this (idk how I want to go about this yet)
 culling??? (maybe just use https://gist.github.com/podgorskiy/e698d18879588ada9014768e3e82a644, but it does use aabb)
 pcf filtering for shadows
 cursors
 skybox
 nlips?

Bugfixxy stuff
 get lod working
 fix resizing
 get the api inspector figured out (I am crashing on vkEnumeratePhysicalDevices)
 selection is busted
 cleanup code
*/