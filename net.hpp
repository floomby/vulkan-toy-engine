#pragma once

#include <atomic>
#include <thread>

#include <boost/asio.hpp>

#include "state.hpp"

namespace Networking { class Client; }

class Net {
public:
    enum class Mode {
        SERVER,
        CLIENT
    };

    Net();
    ~Net();

    void stateUpdater();
    void bindStateUpdater(AuthoritativeState *state, Mode mode, Networking::Client *client = nullptr);
    void launchIo();

    boost::asio::io_context io;
    std::atomic<bool> done = false;
    
    static constexpr uint msPerTick = 33;
    static constexpr float ticksPerSecond = 1000.0f / (float)msPerTick;
    static constexpr float secondsPerTick = 1.0f / ticksPerSecond;
private:
    Mode mode;
    boost::asio::steady_timer timer;
    std::thread ioThread;
    AuthoritativeState *state;
};

#include "api.hpp"

namespace Networking {

enum { MAX_LENGTH = 1024 };

static_assert(MAX_LENGTH > sizeof(ApiProtocol), "Networking buffer size too small");

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket&& socket);

    void start();
    void doRead();
    // void doWrite(size_t size);
    void writeData(const ApiProtocol& data);
private:
    boost::asio::ip::tcp::socket socket;
    char data[MAX_LENGTH];
};

class Server {
public:
    Server(boost::asio::io_context& ioContext, short port);
    void writeData(const ApiProtocol& data);
    std::vector<std::shared_ptr<Session>> sessions;
private:
    void doAccept();
    boost::asio::ip::tcp::acceptor acceptor;
};

class Client {
public:
    Client(boost::asio::io_context& ioContext, const std::string& hostname, const std::string& port);
    void writeData(const ApiProtocol& data);
    void bindToState(AuthoritativeState *state, std::atomic<bool> *done);
private:
    boost::asio::ip::tcp::socket socket;
    char data[MAX_LENGTH];
};

}