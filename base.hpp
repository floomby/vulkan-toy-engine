#pragma once

#include "utilities.hpp"
#include "net.hpp"
#include "lua_wrapper.hpp"

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
    friend void AuthoritativeState::process(ApiProtocol *data, std::optional<std::shared_ptr<Networking::Session>>);
    friend void LuaWrapper::dispatchCallbacks();
    friend void AuthoritativeState::doUpdateTick();
    bool headless = false;
protected:
    AuthoritativeState authState;
    Gui *gui = nullptr;
    Sound *sound = nullptr;
    Scene *currentScene;
    Net net;
};