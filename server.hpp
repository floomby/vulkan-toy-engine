#pragma once

#include "base.hpp"

class Server : public Base {
    friend class Networking::Server;
public:
    Server();
    ~Server();
    virtual void runCurrentScene();
    virtual void send(const ApiProtocol& data);
    virtual void send(ApiProtocol&& data);
    virtual void quit();
private:
    void poll();
    void poll_();
    std::shared_ptr<Networking::Server> server;
};