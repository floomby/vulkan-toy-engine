#pragma once

#include "base.hpp"

class Server : public Base {
public:
    Server();
    ~Server();
    virtual void runCurrentScene();
    virtual void send(const ApiProtocol& data);
    virtual void send(ApiProtocol&& data);
private:
    void poll();
    std::shared_ptr<Networking::Server> server;
};