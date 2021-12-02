#pragma once

#include "base.hpp"

class Server : public Base {
public:
    Server();
    virtual void runCurrentScene();
private:
    void poll();
    std::thread pollThread;
    std::atomic<bool> done;
};