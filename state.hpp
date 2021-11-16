#pragma once

#include <list>
#include <map>
#include <mutex>
#include <stdint.h>
#include <thread>
#include <utility>

#include "instance.hpp"

// struct StateSettings {
    
// };

// class State;

// class StatePropagator {
// public:
//     StatePropagator(State&& state);
//     std::atomic<bool> completed;
//     inline void doTicks(uint ticks) {
//         std::scoped_lock(ticksToDoMutex);
//         ticksToDo += ticks;
//     }
//     void dumpStateAfterCompletion(std::atomic<State *>& state);
// private:
//     uint ticksToDo = 0;
//     std::mutex ticksToDoMutex;
//     std::thread proscessingThread;
// };

#include <csignal>

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

// I feel like I am doing this all wrong
class AuthoritativeState {
public:
    std::vector<Instance> instances;
    uint totalElapsedTicks;
    void doUpdateTick();
    InstanceID counter = 100;
    std::mutex lock;
    inline std::vector<Instance>::iterator getInstance(InstanceID id) {
        return find_if(instances.begin(), instances.end(), [id](auto x) -> bool { return x.id == id; });
    }
    std::vector<Team> teams;
};

class ObservableState {
public:
    std::vector<Instance> instances;
    uint totalElapsedTicks;
    uint commandCount(const std::vector<uint>& which);
    CommandGenerator<CommandCoroutineType> getCommandGenerator(std::vector<uint> *which);
    // player economy state and metadata stuff and so forth....
    // Updating does not really belong here
    void doUpdate(float timeDelta);
    void syncToAuthoritativeState(AuthoritativeState& state);
};



// class State {
// public:
//     void insertCommand(Command&& command, uint when);
//     void step();
//     inline void observe(std::function<void (ObservableState&)> observer) {
//         std::lock_guard guard(observableStateMutex);
//         observer(observableState);
//     }
// private:
//     // CommandList commandList;
//     std::mutex commandListMutex;
//     uint atTicks;
//     ObservableState observableState;
//     std::mutex observableStateMutex;
// };