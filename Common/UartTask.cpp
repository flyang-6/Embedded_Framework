#include "UartTask.hpp"
#include <cstring>
#include "Logger.hpp"

UartTask::UartTask(IUart* port) : uart_port_(port) {
    //
    rx_queue_ = xQueueCreate(10, sizeof(UartMessage));
}

UartTask::~UartTask() {
    if (rx_queue_) {
        vQueueDelete(rx_queue_);
    }
}

ErrorCode UartTask::Start() {
    if (rx_queue_ == nullptr) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    
    //
    BaseType_t res = xTaskCreate(TaskEntry, "UartTask", 512, this, 3, nullptr);
    if (res != pdPASS) {
        return ErrorCode::ERR_CHECKSUM_FAIL;
    }
    return ErrorCode::OK;
}

//
void UartTask::PushDataFromISR(const uint8_t* data, uint16_t len) {
    UartMessage msg;
    msg.length = len > sizeof(msg.data) ? sizeof(msg.data) : len;
    memcpy(msg.data, data, msg.length);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    //
    xQueueSendFromISR(rx_queue_, &msg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void UartTask::TaskEntry(void* pvParameters) {
    UartTask* instance = static_cast<UartTask*>(pvParameters);
    instance->Run();
}

void UartTask::Run() {
    UartMessage msg;
    while (true) {
        //
        if (xQueueReceive(rx_queue_, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
            //
            Logger::Info("UartTask", "Received data from UART!");
        } else {
            // Timeout handling (do nothing)
        }
    }
}