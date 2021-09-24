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
 Cpu allocator (vma)
 Get lod working
 game instance allocations
 game state
 ECS? (or something else as an organizational structure for the proccessing)
 Blinn-Phong
 lt - better model library (assimp?)
 lt - emmisivity maps
 makefile build faster (probably some code changes needed though to not have to recomplile so much temlate code every time)
 2d for hud
 culling (maybe just use https://gist.github.com/podgorskiy/e698d18879588ada9014768e3e82a644, but it does use aabb)
 lt - shadow render pass
 fix crashing on bad vulkan call (tries to free stuff that has not been allocated yet when doing stack unwinding)
*/