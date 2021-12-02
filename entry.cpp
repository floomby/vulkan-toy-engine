#include "engine.hpp"
#include "server.hpp"

#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>

namespace po = boost::program_options;
namespace prc = boost::process;

void run(EngineSettings& settings);

int main(int argc, char **argv) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Print help message")
        ("debug", "Enable extra debug options")
        ("use-one-queue", "Disable use of the transfer queue")
        ("rebuild-font-cache", "Forcefuly rebuild the font cache")
        ("server", "Spawn a server")
        ("is-server", "Run headless as a server instance")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    prc::child serverProcess;
    prc::opstream ops;

    if (vm.count("is-server")) {
        std::cout << "Running server" << std::endl;
        Server server;
        server.runCurrentScene();
        return EXIT_SUCCESS;
    }

    if (vm.count("server")) {
        std::string str = argv[0];
        str += " --is-server";
        serverProcess = prc::child(str, prc::std_in < ops);
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
    settings.rebuildFontCache = vm.count("rebuild-font-cache");

    try {
        run(settings);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        if (vm.count("server")) {
            ops << "quit" << std::endl;
            std::cout << "Waiting for server to shut down..." << std::endl;
        }
        serverProcess.wait();
        return EXIT_FAILURE;
    }

    if (vm.count("server")) {
        ops << "quit" << std::endl;
        std::cout << "Waiting for server to shut down..." << std::endl;
    }
    serverProcess.wait();
    return EXIT_SUCCESS;
}

// I need raii pattern here before the catch cause of the exception handling interaction with the destructor
void run(EngineSettings& settings) {
    Engine engine(settings);
    engine.init();
    engine.runCurrentScene();
}