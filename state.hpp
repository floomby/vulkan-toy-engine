#pragma once

#include <list>
#include <mutex>
#include <stdint.h>
#include <thread>
#include <utility>

#include "instance.hpp"

enum class CommandKind {
    MOVE,
    ATTACK,
    //.....
};

struct CommandData {
    glm::vec3 target;
};

struct Command {
    CommandKind kind;
    InstanceID id;
    CommandData data;
};

typedef std::list<std::pair<Command, uint32_t>> CommandList;

// struct StateSettings {
    
// };

class State;

class StatePropagator {
public:
    StatePropagator(State&& state);
    std::atomic<bool> completed;
    inline void doTicks(uint32_t ticks) {
        std::scoped_lock(ticksToDoMutex);
        ticksToDo += ticks;
    }
    void dumpStateAfterCompletion(std::atomic<State *>& state);
private:
    uint32_t ticksToDo = 0;
    std::mutex ticksToDoMutex;
    std::thread proscessingThread;
};

class ObservableState {
public:
    std::vector<Instance> instances;
    uint32_t totalElapsedTicks;
    // player economy state and metadata stuff and so forth....
};

class State {
public:
    void insertCommand(Command&& command, uint32_t when);
    void step();
    inline void observe(std::function<void (ObservableState&)> observer) {
        std::lock_guard guard(observableStateMutex);
        observer(observableState);
    }
private:
    CommandList commandList;
    std::mutex commandListMutex;
    uint32_t atTicks;
    ObservableState observableState;
    std::mutex observableStateMutex;
};