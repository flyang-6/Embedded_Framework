#include "CanEchoTask.hpp"
#include "Logger.hpp"

namespace {
constexpr uint32_t kSelfTestPeriodMs = 5000U;
constexpr uint32_t kSelfTestCanId = 0x123U;
}  // namespace

CanEchoTask::CanEchoTask(ICan& can, BoardLed& led, bool echo_back)
    : can_(can), led_(led), echo_back_(echo_back), queue_(nullptr), count_(0), seq_(0) {
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

// 回环自测：发一帧，CAN 控制器会把它自己收回 FIFO0，走中断接收路径
void CanEchoTask::SendSelfTestFrame() {
    const uint8_t seq = seq_++;
    CanFrame frame;
    frame.id = kSelfTestCanId;
    frame.extended = false;
    frame.dlc = 8;
    for (uint8_t i = 0; i < frame.dlc; ++i) {
        frame.data[i] = static_cast<uint8_t>(seq + i);
    }

    ErrorCode rc = can_.Transmit(frame, 10U);
    if (rc != ErrorCode::OK) {
        Logger::Error("CAN", rc, "self-test tx failed");
        return;
    }
    Logger::Info("CAN", "tx self-test id=0x%03lX seq=%u",
                 static_cast<unsigned long>(frame.id), seq);
}

void CanEchoTask::Run() {
    Logger::Info("CAN", "can task started, echo_back=%d", echo_back_ ? 1 : 0);

    for (;;) {
        CanFrame frame;
        if (xQueueReceive(queue_, &frame, pdMS_TO_TICKS(kSelfTestPeriodMs)) == pdTRUE) {
            ++count_;
            led_.Toggle();
            Logger::Info("CAN", "rx id=0x%03lX dlc=%u ext=%d",
                         static_cast<unsigned long>(frame.id), frame.dlc,
                         frame.extended ? 1 : 0);

            if (echo_back_) {
                ErrorCode rc = can_.Transmit(frame, 10U);
                if (rc != ErrorCode::OK) {
                    Logger::Error("CAN", rc, "echo id=0x%03lX failed",
                                  static_cast<unsigned long>(frame.id));
                }
            }
        } else if (!echo_back_) {
            SendSelfTestFrame();
        }
    }
}
