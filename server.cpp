#include "server.hpp"

#include <iostream>

Server::Server()
: server(Networking::Server(net.io, 5555)) { }

void Server::poll() {
    std::string str;
    std::cin >> str;
    net.done = true;
}

void Server::runCurrentScene() {
    std::thread ioThread(&Server::poll, this);
    try {
        net.bindStateUpdater(&authState, Net::Mode::SERVER);
        net.io.run();
    } catch (const std::exception& e) { 
        std::cerr << e.what() << std::endl;
    }
    ioThread.join();
}

void Server::send(const ApiProtocol& data) {
    server.writeData(data);
}

#include "lua_wrapper.hpp"

Base::Base() : lua(new LuaWrapper(true)), authState(this) {}

Base::~Base() { delete lua; }
