#include "UartEchoTask.hpp"
#include "Logger.hpp"

#include <cstring>

namespace {
struct UartChunk {
    uint16_t len;
    uint8_t data[64];
};
}  // namespace

UartEchoTask::UartEchoTask(IUart& uart, BoardLed& led)
    : uart_(uart), led_(led), queue_(nullptr), frames_(0) {
    queue_ = xQueueCreate(8, sizeof(UartChunk));
}

UartEchoTask::~UartEchoTask() {
    if (queue_ != nullptr) {
        vQueueDelete(queue_);
    }
}

ErrorCode UartEchoTask::Start() {
    if (queue_ == nullptr) {
        return ErrorCode::ERR_NO_MEM;
    }

    ErrorCode rc = uart_.StartReceive(&UartEchoTask::OnRxFromIsr, this);
    if (rc != ErrorCode::OK) {
        return rc;
    }

    if (xTaskCreate(TaskEntry, "UartEcho", 384, this, 3, nullptr) != pdPASS) {
        return ErrorCode::ERR_NO_MEM;
    }
    return ErrorCode::OK;
}

void UartEchoTask::OnRxFromIsr(void* ctx, const uint8_t* data, uint16_t len) {
    UartEchoTask* self = static_cast<UartEchoTask*>(ctx);
    if (self == nullptr || data == nullptr || len == 0U) {
        return;
    }

    UartChunk chunk;
    chunk.len = (len > sizeof(chunk.data)) ? static_cast<uint16_t>(sizeof(chunk.data)) : len;
    std::memcpy(chunk.data, data, chunk.len);

    BaseType_t woken = pdFALSE;
    (void)xQueueSendFromISR(self->queue_, &chunk, &woken);
    portYIELD_FROM_ISR(woken);
}

void UartEchoTask::TaskEntry(void* arg) {
    static_cast<UartEchoTask*>(arg)->Run();
}

void UartEchoTask::Run() {
    for (;;) {
        UartChunk chunk;
        if (xQueueReceive(queue_, &chunk, pdMS_TO_TICKS(1000)) == pdTRUE) {
            ErrorCode rc = uart_.Transmit(chunk.data, chunk.len, 100U);
            if (rc != ErrorCode::OK) {
                Logger::Error("UART", rc, "echo %u bytes failed", chunk.len);
            } else {
                ++frames_;
                led_.Toggle();
                if ((frames_ % 10U) == 0U) {
                    Logger::Info("UART", "echoed %lu frames",
                                 static_cast<unsigned long>(frames_));
                }
            }
        }
        // 1s 收不到数据属于正常空闲，不报错
    }
}
