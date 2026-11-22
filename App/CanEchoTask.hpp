#pragma once
#include "ICan.hpp"
#include "BoardLed.hpp"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

class CanEchoTask final {
public:
    CanEchoTask(ICan& can, BoardLed& led);
    ~CanEchoTask();

    ErrorCode Start();

    static void OnRxFromIsr(void* ctx, const CanFrame& frame);

private:
    static void TaskEntry(void* arg);
    void Run();

    ICan& can_;
    BoardLed& led_;
    QueueHandle_t queue_;
    uint32_t count_;
};
