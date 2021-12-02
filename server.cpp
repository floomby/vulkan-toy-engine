#include "server.hpp"

#include <iostream>

Server::Server()
: server(Networking::Server(net.io, 5555)) {
    pollThread = std::thread(&Server::poll, this);
}

void Server::poll() {
    std::string str;
    std::cin >> str;
    net.done = true;
}

void Server::runCurrentScene() {
    try {
        net.io.run();
    } catch (const std::exception& e) { 
        std::cerr << e.what() << std::endl;
    }
    pollThread.join();
}

void Server::send(const ApiProtocol& data) {
    server.writeData(data);
}

#include "lua_wrapper.hpp"

Base::Base() : lua(new LuaWrapper(true)) {}

Base::~Base() { delete lua; }
