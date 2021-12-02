#include "net.hpp"

#include <chrono>

namespace ip = boost::asio::ip;

Net::Net() {
    toExit = false;
    unprocessedSeverTicks = 0;
}

Net::~Net() {
    toExit = true;
    try {
        pretendServerThread.join();
    } catch(const std::exception& e) {}
}

void Net::start() {
    pretendServerThread = std::thread(&Net::ticker, this);
}

void Net::ticker() {
    while (!toExit) {
        unprocessedSeverTicks++;
        std::this_thread::sleep_for(std::chrono::milliseconds(msPerTick));
    }
}

#include <iostream>

namespace Networking {

Session::Session(ip::tcp::socket&& socket)
: socket(std::move(socket)) {}

void Session::start() {
    doRead();
}

void Session::doRead() {
    auto self(shared_from_this());
    socket.async_read_some(boost::asio::buffer(data, MAX_LENGTH),
        [this, self](boost::system::error_code err, size_t length) {
            if (!err) {
                // doWrite(length);
                std::cout << reinterpret_cast<ApiProtocol *>(data)->buf << std::endl;
            }
        });
}

void Session::doWrite(size_t size) {
    auto self(shared_from_this());
    boost::asio::async_write(socket, boost::asio::buffer(data, size),
        [this, self](boost::system::error_code err, std::size_t /*length*/) {
            if (!err) {
                doRead();
            }
        });
}

void Session::writeData(const ApiProtocol& data) {
    boost::asio::write(socket, boost::asio::buffer(&data, sizeof(ApiProtocol)));
}

Server::Server(boost::asio::io_context& ioContext, short port)
: acceptor(ioContext, ip::tcp::endpoint(ip::tcp::v4(), port)) {
    doAccept();
}

void Server::doAccept() {
    acceptor.async_accept(
        [this](boost::system::error_code err, ip::tcp::socket socket) {
            if (!err) {
                auto ses = std::make_shared<Session>(std::move(socket));
                sessions.push_back(ses);
                ses->start();
            }

            doAccept();
        });
}

void Server::writeData(const ApiProtocol& data) {
    for (const auto& ses : sessions) {
        ses->writeData(data);
    }
}

Client::Client(boost::asio::io_context& ioContext, const std::string& hostname, const std::string& port)
: socket(ioContext) {
    ip::tcp::resolver resolver(ioContext);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    boost::asio::connect(socket, resolver.resolve(hostname, port));
}

void Client::writeData(const ApiProtocol& data) {
    boost::asio::write(socket, boost::asio::buffer(&data, sizeof(ApiProtocol)));
}

}