#pragma once

#include <atomic>
#include <thread>
#include <queue>

#include <boost/asio.hpp>

#include "state.hpp"
#include "econf.h"

namespace Networking { class Client; class Server; }

class Net {
public:
    enum class Mode {
        SERVER,
        CLIENT
    };

    Net();
    ~Net();

    void stateUpdater();
    void bindStateUpdater(AuthoritativeState *state, std::shared_ptr<Networking::Client> client);
    void bindStateUpdater(AuthoritativeState *state, std::shared_ptr<Networking::Server> server);
    void launchIo();

    boost::asio::io_context io;
    std::atomic<bool> done = false;
private:
    std::shared_ptr<Networking::Server> server;
    Mode mode;
    boost::asio::steady_timer timer;
    std::thread ioThread;
    AuthoritativeState *state;
};

#include "api.hpp"

namespace Networking {

// enum { MAX_LENGTH = 1024 };

// static_assert(MAX_LENGTH > sizeof(ApiProtocol), "Networking buffer size too small");

class Server;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket&& socket, std::weak_ptr<Server> server);

    void start();
    void doRead();
    void writeData(const ApiProtocol& data);
    std::shared_ptr<Team> team;
private:
    boost::asio::ip::tcp::socket socket;
    char data[sizeof(ApiProtocol)];
    std::weak_ptr<Server> server;
};

class Server : public std::enable_shared_from_this<Server> {
public:
    Server(boost::asio::io_context& ioContext, short port);
    void writeData(const ApiProtocol& data);
    std::vector<std::shared_ptr<Session>> sessions;
    void removeSession(std::shared_ptr<Session> session);
    friend void Session::doRead();
    friend void Net::stateUpdater();
private:
    void doAccept(std::weak_ptr<Server> self);
    boost::asio::ip::tcp::acceptor acceptor;
    std::queue<std::pair<ApiProtocol, std::shared_ptr<Session>>> queue;
};

// TODO The client has a use after free bug when shutting down
class Client : public std::enable_shared_from_this<Client> {
public:
    Client(boost::asio::io_context& ioContext, const std::string& hostname, const std::string& port, AuthoritativeState *state);
    void writeData(const ApiProtocol& data);
    void doRead();
private:
    void bindToState(AuthoritativeState *state);
    boost::asio::ip::tcp::socket socket;
    char data[sizeof(ApiProtocol)];
    boost::asio::io_context *ioContext;
};

}