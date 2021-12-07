#pragma once

#include "utilities.hpp"
#include "net.hpp"

class Api;
class Gui;
class Scene;
class LuaWrapper;
class Sound;

#include "state.hpp"

class Base {
    friend class Api;
public:
    Base();
    ~Base();
    virtual void runCurrentScene() = 0;
    virtual void send(const ApiProtocol& data) = 0;
    virtual void send(ApiProtocol&& data) = 0;
    virtual void quit() = 0;
    LuaWrapper *lua;
    friend void AuthoritativeState::process(ApiProtocol *data);
    bool headless = false;
protected:
    Gui *gui = nullptr;
    Sound *sound = nullptr;
    AuthoritativeState authState;
    std::mutex apiLock;
    Scene *currentScene;
    Net net;
};