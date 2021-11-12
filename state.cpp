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

void ObservableState::doUpdate(float timeDelta) {
    return;
    for (auto& inst : instances) {
        if (!inst.commandList.empty()) {
            const auto& cmd = inst.commandList.front();
            float len;
            glm::vec3 delta;
            switch (cmd.kind){
                case CommandKind::MOVE:
                    delta = cmd.data.dest - inst.position;
                    len = length(delta);
                    if (len > inst.entity->maxSpeed / 30.0f) {
                        inst.position += (inst.entity->maxSpeed * timeDelta / len) * delta;
                    } else {
                        inst.position = cmd.data.dest;
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

void ObservableState::syncToAuthoritativeState(AuthoritativeState& state) {
    uint highestSynced = 0;
    uint syncIndex = 0;
    for (int i = 0; i < instances.size(); i++) {
        if (instances[i].inPlay) {
            if (state.instances[syncIndex].id > instances[i].id) {
                instances[i].orphaned = true;
            } else if (state.instances[syncIndex].id == instances[i].id) {
                instances[i].syncToAuthInstance(state.instances[syncIndex]);
                syncIndex++;
            } else {
                std::cerr << "Instance syncronization problem" << std::endl;
            }
        }
    }
    for (; syncIndex < state.instances.size(); syncIndex++) {
        instances.push_back(state.instances[syncIndex]);
    }
}

#include "pathgen.hpp"

void AuthoritativeState::doUpdateTick() {
    const float timeDelta = 33.0f / 1000.0f;
    for (auto& inst : instances) {
        if (!inst.commandList.empty()) {
            const auto& cmd = inst.commandList.front();
            // float len;
            // glm::vec3 delta;
            switch (cmd.kind){
                case CommandKind::MOVE:
                    // delta = cmd.data.dest - inst.position;
                    // len = length(delta);
                    // if (len > inst.entity->maxSpeed * timeDelta) {
                    //     inst.position += (inst.entity->maxSpeed * timeDelta / len) * delta;
                    // } else {
                    //     inst.position = cmd.data.dest;
                    //     inst.commandList.pop_front();
                    // }
                    Pathgen::arrive(inst, cmd.data.dest);
                    break;
                case CommandKind::STOP:
                    inst.commandList.clear();
                    break;
            }
        }
    }
}