#include "net.hpp"

#include <chrono>

Net::Net() {
    toExit = false;
    unprocessedSeverTicks = 0;
}

Net::~Net() {
    toExit = true;
    pretendServerThread.join();
}

void Net::start() {
    pretendServerThread = std::thread(&Net::ticker, this);
}

void Net::ticker() {
    while (!toExit) {
        unprocessedSeverTicks++;
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}