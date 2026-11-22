#include "CanEchoTask.hpp"
#include "Logger.hpp"

CanEchoTask::CanEchoTask(ICan& can, BoardLed& led)
    : can_(can), led_(led), queue_(nullptr), count_(0) {
    queue_ = xQueueCreate(8, sizeof(CanFrame));
}

CanEchoTask::~CanEchoTask() {
    if (queue_ != nullptr) {
        vQueueDelete(queue_);
    }
}

ErrorCode CanEchoTask::Start() {
    if (queue_ == nullptr) {
        return ErrorCode::ERR_NO_MEM;
    }

    ErrorCode rc = can_.StartReceive(&CanEchoTask::OnRxFromIsr, this);
    if (rc != ErrorCode::OK) {
        return rc;
    }

    if (xTaskCreate(TaskEntry, "CanEcho", 320, this, 2, nullptr) != pdPASS) {
        return ErrorCode::ERR_NO_MEM;
    }
    return ErrorCode::OK;
}

void CanEchoTask::OnRxFromIsr(void* ctx, const CanFrame& frame) {
    CanEchoTask* self = static_cast<CanEchoTask*>(ctx);
    if (self == nullptr) {
        return;
    }

    BaseType_t woken = pdFALSE;
    (void)xQueueSendFromISR(self->queue_, &frame, &woken);
    portYIELD_FROM_ISR(woken);
}

void CanEchoTask::TaskEntry(void* arg) {
    static_cast<CanEchoTask*>(arg)->Run();
}

void CanEchoTask::Run() {
    for (;;) {
        CanFrame frame;
        if (xQueueReceive(queue_, &frame, portMAX_DELAY) == pdTRUE) {
            ++count_;
            led_.Toggle();
            Logger::Info("CAN", "rx id=0x%03lX dlc=%u ext=%d",
                         static_cast<unsigned long>(frame.id), frame.dlc,
                         frame.extended ? 1 : 0);

            ErrorCode rc = can_.Transmit(frame, 10U);
            if (rc != ErrorCode::OK) {
                Logger::Error("CAN", rc, "echo id=0x%03lX failed",
                              static_cast<unsigned long>(frame.id));
            }
        }
    }
}
