#pragma once
#include "ICan.hpp"
#include "BoardLed.hpp"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

class CanEchoTask final {
public:
    // echo_back = true  ：收到帧再发一遍（真实总线上做回显）
    // echo_back = false ：只收不回，并每 5 s 自发一帧做回环自测
    CanEchoTask(ICan& can, BoardLed& led, bool echo_back);
    ~CanEchoTask();

    ErrorCode Start();

    static void OnRxFromIsr(void* ctx, const CanFrame& frame);

private:
    static void TaskEntry(void* arg);
    void Run();
    void SendSelfTestFrame();

    ICan& can_;
    BoardLed& led_;
    bool echo_back_;
    QueueHandle_t queue_;
    uint32_t count_;
    uint8_t seq_;
};
