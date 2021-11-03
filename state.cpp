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

/*
    return [this, which] () {
        auto v = which;
        auto vit = which.data();
        auto it = this->instances[*vit].commandList.begin();
        return [=, this] () mutable -> Command {
            while(true) {
                if (it != this->instances[*vit].commandList.end()) {
                    return *it++;
                } else {
                    this->instances[*++vit].commandList.begin();
                }
            }
        };
    }();
*/