#pragma once

#include <atomic>
#include <thread>

#include <boost/asio.hpp>

#include "state.hpp"

class Net {
public:
    Net();
    ~Net();
    void start();
    std::atomic<int> unprocessedSeverTicks;
    static constexpr uint msPerTick = 33;
    static constexpr float ticksPerSecond = 1000.0f / (float)msPerTick;
    static constexpr float secondsPerTick = 1.0f / ticksPerSecond;
    // std::mutex lock;

    boost::asio::io_context io;
    std::atomic<bool> done = false;
private:

    std::atomic<bool> toExit;
    std::thread pretendServerThread;
    void ticker();
};

#include "api.hpp"

namespace Networking {

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket&& socket);

    void start();
    void doRead();
    void doWrite(size_t size);
    void writeData(const ApiProtocol& data);
private:
    enum { MAX_LENGTH = 1024 };
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
private:
    boost::asio::ip::tcp::socket socket;
};

}