#include "server.hpp"

#include <iostream>

Server::Server() {
    pollThread = std::thread(&Server::poll, this);
}

void Server::poll() {
    std::string str;
    // while (!done) {
        std::cin >> str;
        done = true;
        // if (str == "quit") done = true;
    // }
}

void Server::runCurrentScene() {
    // int flags = fcntl(0, F_GETFL, 0);
    // fcntl(0, F_SETFL, flags | O_NONBLOCK);
    while (!done) {
        try {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            std::cout << "doing thing" << std::endl;
        } catch (const std::exception& e) { 
            std::cerr << e.what() << std::endl;
        }
    }
    pollThread.join();
}

#include "lua_wrapper.hpp"

Base::Base() : lua(new LuaWrapper(true)) {}

Base::~Base() { delete lua; }
