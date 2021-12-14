#include "net.hpp"
#include "server.hpp"

#include <boost/bind/bind.hpp>
#include <chrono>
#include <iostream>

namespace ip = boost::asio::ip;

Net::Net()
: timer(io, boost::asio::chrono::milliseconds(Config::Net::msPerTick)) { }

Net::~Net() {
    if (ioThread.joinable()) {
        std::cout << "Waiting to join net thread..." << std::endl;
        ioThread.join();
    }
}

// Server state update
void Net::stateUpdater() {
    while (!server->queue.empty()) {
        auto msg = server->queue.front();
        state->process(&msg.first, msg.second);
        state->emit(msg.first);
        state->doCallbacks();
        server->queue.pop();
    }
    if (!state->paused) {
        state->doUpdateTick();
        ApiProtocol advance = { ApiProtocolKind::FRAME_ADVANCE, state->frame };
        state->emit(advance);
    }
    if (done) { io.stop(); return; }
    timer.expires_at(timer.expiry() + boost::asio::chrono::milliseconds(Config::Net::msPerTick));
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
                std::shared_ptr(server)->queue.push({ *reinterpret_cast<ApiProtocol *>(data), shared_from_this() });
                doRead();
            }
        });
}

void Session::writeData(const ApiProtocol& data) {
    try {
        boost::asio::write(socket, boost::asio::buffer(&data, sizeof(ApiProtocol)));
    } catch (const std::exception& e) { 
        std::cerr << "Unable to write to socket: " << e.what();
        if (team) {
            std::cerr << " (" << (uint)team->id << " " << team->displayName << ")"; 
        }
        std::cerr << std::endl;
        std::shared_ptr(server)->removeSession(shared_from_this());
    }
}

void Server::removeSession(std::shared_ptr<Session> session) {
    auto it = find(sessions.begin(), sessions.end(), session);
    assert(it != sessions.end());
    sessions.erase(it);
}

Server::Server(boost::asio::io_context& ioContext, short port)
: acceptor(ioContext, ip::tcp::endpoint(ip::tcp::v4(), port)) {
    ioContext.post([this](){ doAccept(weak_from_this()); });
}

void Server::doAccept(std::weak_ptr<Server> self) {
    acceptor.async_accept(
        [this, self](boost::system::error_code err, ip::tcp::socket socket) {
            if (static_cast<::Server *>(Api::context)->authState.started()) {
                try {
                    ApiProtocol data = { ApiProtocolKind::SERVER_MESSAGE, 0, "Error: Unable to connect to alread started game." };
                    boost::asio::write(socket, boost::asio::buffer(&data, sizeof(ApiProtocol)));
                } catch (const std::exception& e) { 
                    std::cerr << "Unable to write to socket: " << e.what() << std::endl;
                }
            } else if (!err) {
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
        state->enableCallbacks();
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
            state->process(reinterpret_cast<ApiProtocol *>(self->data), {});
            self->bindToState(state);
        } else std::cerr << "Networking error: " << err << " thread id: " << std::this_thread::get_id() << std::endl;
    });
}

}