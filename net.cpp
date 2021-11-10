#include "net.hpp"

#include <chrono>

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