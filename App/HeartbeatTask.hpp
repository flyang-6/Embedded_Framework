#pragma once
#include "ITimer.hpp"
#include "BoardLed.hpp"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

class HeartbeatTask final {
public:
    HeartbeatTask(ITimer& timer, BoardLed& led, uint32_t period_ms);
    ~HeartbeatTask();

    ErrorCode Start();

private:
    static void TaskEntry(void* arg);
    static void OnTimerFromIsr(void* ctx);
    void Run();

    ITimer& timer_;
    BoardLed& led_;
    uint32_t period_ms_;
    SemaphoreHandle_t tick_;
    uint32_t ticks_;
};
