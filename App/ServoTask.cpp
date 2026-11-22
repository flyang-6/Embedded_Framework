#include "ServoTask.hpp"
#include "Logger.hpp"

ServoTask::ServoTask(Servo& pan, Servo& tilt) : pan_(pan), tilt_(tilt) {}

ErrorCode ServoTask::Start() {
    if (xTaskCreate(TaskEntry, "Servo", 320, this, 1, nullptr) != pdPASS) {
        return ErrorCode::ERR_NO_MEM;
    }
    return ErrorCode::OK;
}

void ServoTask::TaskEntry(void* arg) {
    static_cast<ServoTask*>(arg)->Run();
}

void ServoTask::Run() {
    Logger::Info("SERVO", "servo task started, sweep 0..180 step 15 every 400ms");

    for (;;) {
        for (int16_t deg = 0; deg <= 180; deg += 15) {
            ErrorCode rc1 = pan_.SetAngle(deg);
            ErrorCode rc2 = tilt_.SetAngle(180 - deg);   // 两个舵机反向摆
            if (rc1 != ErrorCode::OK) {
                Logger::Error("SERVO", rc1, "pan set %d deg failed", deg);
            }
            if (rc2 != ErrorCode::OK) {
                Logger::Error("SERVO", rc2, "tilt set %d deg failed", 180 - deg);
            }
            vTaskDelay(pdMS_TO_TICKS(400));
        }
    }
}
