#include "server.hpp"

#include <iostream>

#include "scene.hpp"

Server::Server()
: server(std::make_shared<Networking::Server>(net.io, 5555)) {
    headless = true;
    currentScene = new Scene(this);
    Api::context = this;
}

Server::~Server() {
    delete currentScene;
}

void Server::poll() {
    std::string str;
    std::cin >> str;
    net.done = true;
}

void Server::runCurrentScene() {
    std::thread ioThread(&Server::poll, this);
    try {
        net.bindStateUpdater(&authState, server);
        net.io.run();
    } catch (const std::exception& e) { 
        std::cerr << e.what() << std::endl;
    }
    ioThread.join();
}

void Server::send(const ApiProtocol& data) {
    server->writeData(data);
}

void Server::send(ApiProtocol&& data) {
    server->writeData(data);
}

#include "lua_wrapper.hpp"

Base::Base() : lua(new LuaWrapper(true)), authState(this) {}

Base::~Base() { delete lua; }
