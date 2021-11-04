#include <coroutine>

#include "state.hpp"

uint ObservableState::commandCount(const std::vector<uint>& which) {
    uint acm = 0;
    for (auto& idx : which) {
        // Don't worry size has constant time complexity as of c++11, it does not need to walk the list
        acm += instances[idx].commandList.size();
    }
    return acm;
}

#include <iostream>

CommandGenerator<CommandCoroutineType> ObservableState::getCommandGenerator(std::vector<uint> *which) {
    uint idx = 0;
    auto it = this->instances[(*which)[idx]].commandList.begin();
    while(true) {
        if (it != this->instances[(*which)[idx]].commandList.end()) {
            co_yield CommandCoroutineType(&(*it), it, idx, which);
            it++;
        } else {
            it = this->instances[(*which)[++idx]].commandList.begin();
        }
    }
}

void ObservableState::doUpdateTick() {
    for (auto& inst : instances) {
        if (!inst.commandList.empty()) {
            const auto& cmd = inst.commandList.front();
            float len;
            glm::vec3 delta;
            switch (cmd.kind){
                case CommandKind::MOVE:
                    delta = cmd.data.dest - inst.position;
                    len = delta.length();
                    if (len > inst.entity->maxSpeed) {
                        inst.position += inst.entity->maxSpeed / len * delta;
                    } else {
                        inst.commandList.pop_front();
                    }
                    break;
                case CommandKind::STOP:
                    inst.commandList.clear();
                    break;
            }
        }
    }
}