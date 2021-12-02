#include "net.hpp"

#include <boost/bind/bind.hpp>
#include <chrono>
#include <iostream>

namespace ip = boost::asio::ip;

Net::Net()
: timer(io, boost::asio::chrono::milliseconds(msPerTick)) { }

Net::~Net() {
    if (ioThread.joinable()) {
        std::cout << "Joining net thread..." << std::endl;
        ioThread.join();
    }
}

void Net::stateUpdater() {
    // std::cout << "!!!!!" << std::endl;
    state->doUpdateTick();
    if (done) return;
    timer.expires_at(timer.expiry() + boost::asio::chrono::milliseconds(msPerTick));
    timer.async_wait(boost::bind(&Net::stateUpdater, this));
}

void Net::bindStateUpdater(AuthoritativeState *state) {
    this->state = state;
    timer.async_wait(boost::bind(&Net::stateUpdater, this));
}

void Net::launchIo() {
    ioThread = std::thread([this](){ io.run(); });
}

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