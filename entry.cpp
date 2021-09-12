#include "engine.hpp"

#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

int main(int argc, char **argv) {

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Print help message")
        ("debug", "Enable extra debug options")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    EngineSettings settings = {};
    settings.useConcurrentTransferQueue = true;
    settings.height = 600;
    settings.width = 800;
    settings.validationLayers = { "VK_LAYER_KHRONOS_validation" };
    settings.maxFramesInFlight = 3;
    settings.applicationName = "Vulkan Test";
    settings.verbose = true;
    if (vm.count("debug")) {
        settings.validationExtentions = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
    } else {
        settings.validationExtentions = { };
    }

    Engine engine(settings);
    try {
        engine.init();
        engine.runScene(engine.initScene(/* something */));
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}