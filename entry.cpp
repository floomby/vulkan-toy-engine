#include "engine.hpp"
#include "server.hpp"

#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>

namespace po = boost::program_options;
namespace prc = boost::process;

void run(EngineSettings& settings);

void dupPipeToStdOut(prc::ipstream *ips, ConsoleColorCode mods) {
    std::string buf;
    while(getline(*ips, buf)) {
        // avoid interleaving output
        std::stringstream ss;
        ss << "\033[" << static_cast<int>(mods) << "m" << buf << "\033[0m\n";
        std::cout << ss.str() << std::flush;
    }
}

int main(int argc, char **argv) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Print help message")
        ("debug", "Enable extra debug options")
        ("use-one-queue", "Disable use of the transfer queue")
        ("rebuild-font-cache", "Forcefuly rebuild the font cache")
        ("server", "Spawn a server")
        ("is-server", "Run headless as a server instance")
        ("team", po::value<std::string>(), "set team in the form <display name>,<number>")
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
    prc::ipstream ips, eps;

    if (vm.count("is-server")) {
        std::cout << "Running server" << std::endl;
        Server server;
        server.runCurrentScene();
        // while (true) std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        return EXIT_SUCCESS;
    }

    std::thread epst, ipst;

    if (vm.count("server")) {
        std::string str = argv[0];
        str += " --is-server";
        serverProcess = prc::child(str, prc::std_in < ops, prc::std_out > ips, prc::std_err > eps);

        ipst = std::thread(&dupPipeToStdOut, &ips, ConsoleColorCode::FG_GREEN);
        epst = std::thread(&dupPipeToStdOut, &eps, ConsoleColorCode::FG_RED);
    }

    EngineSettings settings = {};
    if (vm.count("team")) {
        auto str = vm["team"].as<std::string>();
        std::string displayName, teamNumber;
        auto it = str.begin();
        while (it != str.end() && *it != ',')
            displayName.push_back(*it++);
        if (*it == ',') it++;
        while (it != str.end())
            teamNumber.push_back(*it++);
        if (teamNumber.empty() || displayName.empty()) {
            std::cerr << "Invalid team specification" << std::endl;
            return EXIT_FAILURE;
        }
        TeamID id = (TeamID)std::stoi(teamNumber);
        settings.teamIAm = Team(id, displayName);
    }
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
            serverProcess.wait();
            ipst.join();
            epst.join();
        }
        return EXIT_FAILURE;
    }

    if (vm.count("server")) {
        ops << "quit" << std::endl;
        std::cout << "Waiting for server to shut down..." << std::endl;
        serverProcess.wait();
        ipst.join();
        epst.join();
    }
    return EXIT_SUCCESS;
}

// I need raii pattern here before the catch cause of the exception handling interaction with the destructor
void run(EngineSettings& settings) {
    Engine engine(settings);
    engine.init();
    try {
        engine.runCurrentScene();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        throw e;
    }
}

// Note to myself cause I will forget how I did this
// LD_PRELOAD="/usr/lib/x86_64-linux-gnu/libasan.so.6.0.0 ./dlkeeper.so" ./result --server