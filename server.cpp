#include "server.hpp"

#include <iostream>

#include "scene.hpp"

Server::Server()
: server(std::make_shared<Networking::Server>(net.io, Config::port)) {
    headless = true;
    currentScene = new Scene(this);
    Api::context = this;
}

Server::~Server() {
    delete currentScene;
}

void Server::poll() {
    std::chrono::seconds timeout(5);
    while (!net.done) {
        auto future = std::async(&Server::poll_, this);
        if (future.wait_for(timeout) == std::future_status::ready)
            future.get();
    }
}

void Server::poll_() {
    std::string str;
    std::cin >> str;
    net.done = true;
}

void Server::quit() {
    net.done = true;
}

void Server::runCurrentScene() {
    currentScene->initUnitAIs(lua, "unitai");
    std::thread ioThread(&Server::poll, this);
    try {
        net.bindStateUpdater(&authState, server);
        net.io.run();
    } catch (const std::exception& e) { 
        std::cerr << e.what() << std::endl;
    }
    ioThread.join();
}

void Server::emit(const ApiProtocol& data) {
    server->writeData(data);
}

void Server::emit(ApiProtocol&& data) {
    server->writeData(data);
}

void Server::send(const ApiProtocol& data) {
    std::cout << data << std::endl;
    server->queue.push({ data, nullptr });
}

void Server::send(ApiProtocol&& data) {
    server->queue.push({ data, nullptr });
}

#include "lua_wrapper.hpp"

Base::Base() : lua(new LuaWrapper(true)), authState(this) {
    lua->exportEnumToLua<InsertionMode>();
    lua->exportEnumToLua<IEngage>();
    lua->exportEnumToLua<IntrinicStates>();
    lua->apiExport();
}

Base::~Base() { delete lua; }
