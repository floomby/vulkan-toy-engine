#pragma once

#include <atomic>
#include <thread>

#include <boost/asio.hpp>

#include "state.hpp"

class Net {
public:
    Net();
    ~Net();
    void start();
    std::atomic<int> unprocessedSeverTicks;
    static constexpr uint msPerTick = 33;
    static constexpr float ticksPerSecond = 1000.0f / (float)msPerTick;
    static constexpr float secondsPerTick = 1.0f / ticksPerSecond;
    std::mutex lock;

private:
    boost::asio::io_context io;

    std::atomic<bool> toExit;
    std::thread pretendServerThread;
    void ticker();
};