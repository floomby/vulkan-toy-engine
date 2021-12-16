#pragma once

#include <boost/random.hpp>
#include <list>
#include <map>
#include <mutex>
#include <stdint.h>
#include <thread>
#include <utility>

#include "instance.hpp"


struct CommandCoroutineType {
    Command *command;
    std::list<Command>::iterator it;
    uint idx;
    std::vector<uint> *which;
    CommandCoroutineType(Command *command, std::list<Command>::iterator it, uint idx, std::vector<uint> *which)
    : command(command), it(it), idx(idx), which(which) {}
};

template<typename StateType> struct CommandGenerator {
    struct Promise;

    // compiler looks for promise_type
    using promise_type = Promise;
    std::coroutine_handle<Promise> coroutine;
    // using StateType = std::tuple<Command, std::vector<uint>::const_iterator, std::list<Command>::iterator>;

    CommandGenerator(std::coroutine_handle<Promise> handle) : coroutine(handle) {}

    ~CommandGenerator() {
        if(coroutine) coroutine.destroy();
    }

    StateType value() {
        return coroutine.promise().val;
    }

    bool next() {
        coroutine.resume();
        return !coroutine.done();
    }

    struct Promise {
        StateType val;

        Promise() : val({ nullptr, std::list<Command>({}).end(), 0, nullptr }){};
        // Promise(Promise const&) = delete;
        // void operator=(Promise const&) = delete;

        CommandGenerator get_return_object() {
            return CommandGenerator { std::coroutine_handle<Promise>::from_promise(*this) };
        }

        std::suspend_always initial_suspend() {
            return {};
        }

        std::suspend_always yield_value(StateType x) {
            val = x;
            return {};
        }

        std::suspend_never return_void() {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void unhandled_exception() {
            throw;
        }
    };
};

enum class ApiProtocolKind {
    FRAME_ADVANCE,
    SERVER_MESSAGE,
    PAUSE,
    TEAM_DECLARATION,
    COMMAND,
    RESOURCES,
    CALLBACK
};

const uint ApiTextBufferSize = 128;

struct ApiProtocol {
    ApiProtocolKind kind;
    uint64_t frame;
    char buf[ApiTextBufferSize];
    Command command;
    double dbl;
    CallbackID callbackID;
};

#include <iostream>

static const std::vector<std::string> ApiProtocolKinds = enumNames2<ApiProtocolKind>();

namespace std {
    inline ostream& operator<<(ostream& os, const ApiProtocol& data) {
        os << "Kind: " << ApiProtocolKinds[static_cast<int>(data.kind)] << " frame: " << data.frame << " buf: " << data.buf << " " << data.command;
        return os;
    }
}

class Base;

// I feel like I am doing this all wrong
class AuthoritativeState {
public:
    AuthoritativeState(Base *context);
    ~AuthoritativeState();

    std::vector<Instance *> instances;
    std::atomic<unsigned long> frame = 0;
    InstanceID counter = 100;
    std::vector<std::shared_ptr<Team>> teams;

    std::atomic<bool> paused = true;

    void doUpdateTick();
    std::recursive_mutex lock;
    inline std::vector<Instance *>::iterator getInstance(InstanceID id) {
        return find_if(instances.begin(), instances.end(), [id](auto x) -> bool { return x->id == id; });
    }

    bool inline started() { return frame > 0; }
    void dump();
    uint32_t crc();

    void process(ApiProtocol *data, std::optional<std::shared_ptr<Networking::Session>> session);
    void emit(const ApiProtocol& data);
    void doCallbacks();
    void enableCallbacks();

    boost::random::mt11213b randomGenerator { 37946U };
    boost::random::uniform_real_distribution<float> realDistribution { 0.0f, 1.0f };
private:
    std::queue<std::pair<ApiProtocol, std::shared_ptr<Networking::Session>>> callbacks;
    Base *context;
};

class ObservableState {
public:
    std::vector<Instance *> instances;
    uint commandCount(const std::vector<uint>& which);
    CommandGenerator<CommandCoroutineType> getCommandGenerator(std::vector<uint> *which);
    // This update is only for smoothing the animations and stuff (Lag compensation will eventually go in it too once I get around to it)
    void doUpdate(float timeDelta);
    void syncToAuthoritativeState(AuthoritativeState& state);
    std::vector<Team> teams;
};