#include "HeartbeatTask.hpp"
#include "Logger.hpp"

HeartbeatTask::HeartbeatTask(ITimer& timer, BoardLed& led, uint32_t period_ms)
    : timer_(timer), led_(led), period_ms_(period_ms), tick_(nullptr), ticks_(0) {
    tick_ = xSemaphoreCreateBinary();
}

HeartbeatTask::~HeartbeatTask() {
    if (tick_ != nullptr) {
        vSemaphoreDelete(tick_);
        tick_ = nullptr;
    }
}

ErrorCode HeartbeatTask::Start() {
    if (tick_ == nullptr) {
        return ErrorCode::ERR_NO_MEM;
    }
    if (xTaskCreate(TaskEntry, "Heartbeat", 256, this, 1, nullptr) != pdPASS) {
        return ErrorCode::ERR_NO_MEM;
    }
    return timer_.Start(period_ms_, &HeartbeatTask::OnTimerFromIsr, this);
}

void HeartbeatTask::OnTimerFromIsr(void* ctx) {
    HeartbeatTask* self = static_cast<HeartbeatTask*>(ctx);
    if (self == nullptr) {
        return;
    }
    BaseType_t woken = pdFALSE;
    (void)xSemaphoreGiveFromISR(self->tick_, &woken);
    portYIELD_FROM_ISR(woken);
}

void HeartbeatTask::TaskEntry(void* arg) {
    static_cast<HeartbeatTask*>(arg)->Run();
}

void HeartbeatTask::Run() {
    Logger::Info("HB", "heartbeat task started, period=%lums",
                 static_cast<unsigned long>(period_ms_));

    for (;;) {
        // 3 个周期都没等到信号量 → 定时器停了，必须报错
        if (xSemaphoreTake(tick_, pdMS_TO_TICKS(period_ms_ * 3U)) != pdTRUE) {
            Logger::Error("HB", ErrorCode::ERR_TIMEOUT, "timer tick lost");
            continue;
        }

        ++ticks_;
        led_.Toggle();
        if ((ticks_ % 5U) == 0U) {
            Logger::Info("HB", "tick=%lu", static_cast<unsigned long>(ticks_));
        }
    }
}
