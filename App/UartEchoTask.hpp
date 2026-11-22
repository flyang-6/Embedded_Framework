#pragma once
#include "IUart.hpp"
#include "BoardLed.hpp"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

class UartEchoTask final {
public:
    UartEchoTask(IUart& uart, BoardLed& led);
    ~UartEchoTask();

    ErrorCode Start();

    // 中断上下文调用
    static void OnRxFromIsr(void* ctx, const uint8_t* data, uint16_t len);

private:
    static void TaskEntry(void* arg);
    void Run();

    IUart& uart_;
    BoardLed& led_;
    QueueHandle_t queue_;
    uint32_t frames_;
};
