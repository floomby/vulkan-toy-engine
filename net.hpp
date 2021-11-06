#pragma once

#include <atomic>
#include <thread>

class Net {
public:
    Net();
    ~Net();
    void start();
    std::atomic<int> unprocessedSeverTicks;
private:
    std::atomic<bool> toExit;
    std::thread pretendServerThread;
    void ticker();
};