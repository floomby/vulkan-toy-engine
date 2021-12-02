#pragma once

#include "utilities.hpp"
#include "net.hpp"

class Api;
class Gui;
class Scene;
class LuaWrapper;

class Base {
    friend class Api;
public:
    Base();
    ~Base();
    virtual void runCurrentScene() = 0;
    LuaWrapper *lua;
protected:
    Gui *gui = nullptr;
    AuthoritativeState authState;
    std::mutex apiLock;
    Scene *currentScene;
    Net net;
};