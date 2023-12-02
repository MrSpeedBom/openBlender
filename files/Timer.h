#pragma once
#include <chrono>
#include <cstdint>
#include <iostream>

uint64_t timeSinceEpochMillisec()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

namespace Timer {
    uint64_t last_time=0;
    void start() {
        last_time=timeSinceEpochMillisec();
    }
    float current_time() {
        return float(timeSinceEpochMillisec()-last_time)/1000;
    }
};

