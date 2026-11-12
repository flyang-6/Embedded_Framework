#pragma once
#include "IUart.hpp"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

//
struct  UartMessage {
    uint8_t data[64];
    uint16_t length;
};

class UartTask {
private:
    IUart* uart_port_;
    QueueHandle_t rx_queue_;
    
public:
    UartTask(IUart* port);
    ~UartTask();
    
    //
    ErrorCode Start();

    //
    void PushDataFromISR(const uint8_t* data, uint16_t len);

private:
    //
    static void TaskEntry(void* pvParameters);
    void Run();    
};

