#pragma once
#include "Servo.hpp"

#include "FreeRTOS.h"
#include "task.h"

class ServoTask final {
public:
    ServoTask(Servo& pan, Servo& tilt);
    ErrorCode Start();

private:
    static void TaskEntry(void* arg);
    void Run();

    Servo& pan_;
    Servo& tilt_;
};
