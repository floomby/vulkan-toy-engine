#include "engine.hpp"

#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

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

    Engine engine(settings);
    try {
        engine.init();
        engine.runCurrentScene();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
Next steps
 cpu (host) allocation stuff for vulkan (vma)?? (This is probably not worth doing until I know it is bad as it is)
 get lod working
  icons now that I have 2d?
   maybe make an icon for the model
  mipmaps (I still didn't figure this out)
 * game instance allocations (think how do I want to do this?)
 game state (this involves concurrency stuff)
 ECS? (or something else as an organizational structure for the proccessing)
 lt - better model library (assimp?)
 lt - emmisivity maps
 makefile build faster (probably some code changes needed though to not have to recomplile so much temlate code every time)
 gui
  threading (think how to tell the gui object to do things)
  fonts -  I will be uising a library of some sort for this (idk how I want to go about this yet)
 culling??? (maybe just use https://gist.github.com/podgorskiy/e698d18879588ada9014768e3e82a644, but it does use aabb)
 lt - shadow render pass
 there might still be a problem with selections
 there is a bug with resizing again
 hitboxes
 cursors (after hitboxes)
*/