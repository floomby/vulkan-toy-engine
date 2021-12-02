#pragma once

#include "base.hpp"

class Server : public Base {
public:
    Server();
    virtual void runCurrentScene();
    virtual void send(const ApiProtocol& data);
private:
    void poll();
    Networking::Server server;
};