#include "net.hpp"

#include <boost/bind/bind.hpp>
#include <chrono>
#include <iostream>

namespace ip = boost::asio::ip;

Net::Net()
: timer(io, boost::asio::chrono::milliseconds(msPerTick)) { }

Net::~Net() {
    if (ioThread.joinable()) {
        std::cout << "Waiting to join net thread..." << std::endl;
        ioThread.join();
    }
}

// Server state update
void Net::stateUpdater() {
    while (!server->queue.empty()) {
        state->process(&server->queue.front());
        state->emit(server->queue.front());
            ApiProtocol advance = { ApiProtocolKind::FRAME_ADVANCE, state->frame };
            state->emit(advance);
        server->queue.pop();
    }
    state->doUpdateTick();
    // if (!(state->frame % 30)) {
    //     ApiProtocol data { ApiProtocolKind::SERVER_MESSAGE, state->frame };
    //     strcpy(data.buf, "Test server message");
    //     state->emit(data);
    // }
    ApiProtocol advance = { ApiProtocolKind::FRAME_ADVANCE, state->frame };
    state->emit(advance);
    if (done) { io.stop(); return; }
    timer.expires_at(timer.expiry() + boost::asio::chrono::milliseconds(msPerTick));
    timer.async_wait(boost::bind(&Net::stateUpdater, this));
}

// void Net::bindStateUpdater(AuthoritativeState *state, std::shared_ptr<Networking::Client> client) {
//     this->state = state;
//     this->mode = Mode::CLIENT;
//     client->bindToState(state);
// }

void Net::bindStateUpdater(AuthoritativeState *state, std::shared_ptr<Networking::Server> server) {
    this->state = state;
    this->server = server;
    timer.async_wait(boost::bind(&Net::stateUpdater, this));
}

void Net::launchIo() {
    ioThread = std::thread([this](){ io.run(); });
}

namespace Networking {

Session::Session(ip::tcp::socket&& socket, std::weak_ptr<Server> server)
: socket(std::move(socket)), server(server) { }

void Session::start() {
    doRead();
}

void Session::doRead() {
    auto self(shared_from_this());
    boost::asio::async_read(socket, boost::asio::buffer(data, sizeof(ApiProtocol)),
        [this, self](boost::system::error_code err, size_t length) {
            if (!err) {
                // std::cout << "We are here" << std::endl;
                // std::cout << reinterpret_cast<ApiProtocol *>(data)->buf << std::endl;
                server.lock()->queue.push(*reinterpret_cast<ApiProtocol *>(data));
                doRead();
            }
        });
}

void Session::writeData(const ApiProtocol& data) {
    boost::asio::write(socket, boost::asio::buffer(&data, sizeof(ApiProtocol)));
}

Server::Server(boost::asio::io_context& ioContext, short port)
: acceptor(ioContext, ip::tcp::endpoint(ip::tcp::v4(), port)) {
    ioContext.post([this](){ doAccept(weak_from_this()); });
}

void Server::doAccept(std::weak_ptr<Server> self) {
    acceptor.async_accept(
        [this, self](boost::system::error_code err, ip::tcp::socket socket) {
            if (!err) {
                auto ses = std::make_shared<Session>(std::move(socket), self);
                sessions.push_back(ses);
                ses->start();
            }

            doAccept(self);
        });
}

void Server::writeData(const ApiProtocol& data) {
    for (const auto& ses : sessions) {
        ses->writeData(data);
    }
}

Client::Client(boost::asio::io_context& ioContext, const std::string& hostname, const std::string& port, AuthoritativeState *state)
: socket(ioContext), ioContext(&ioContext) {
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    ioContext.post([this, hostname, port, state](){ 
        ip::tcp::resolver resolver(*this->ioContext);
        boost::asio::connect(socket, resolver.resolve(hostname, port));
        bindToState(state);
    });
}

void Client::writeData(const ApiProtocol& data) {
    ioContext->post([data, this](){
        // std::cout << "Doing write from thread " << std::this_thread::get_id() << std::endl;
        boost::asio::async_write(socket, boost::asio::buffer(&data, sizeof(ApiProtocol)),
            [](const boost::system::error_code& err, size_t length) {
                if (err) std::cerr << "Error writing to socket " << err << std::endl;
            });
    });
}

void Client::bindToState(AuthoritativeState *state) {
    auto self(shared_from_this());
    boost::asio::async_read(socket, boost::asio::buffer(data),
    [state, self](boost::system::error_code err, size_t length) {
        if (!err) {
            state->process(reinterpret_cast<ApiProtocol *>(self->data));
            self->bindToState(state);
        } else std::cerr << "Networking error: " << err << " thread id: " << std::this_thread::get_id() << std::endl;
    });
}

}