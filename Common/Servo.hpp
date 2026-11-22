#pragma once
#include <cstdint>
#include "IPwm.hpp"

// SG90 类舵机：50Hz，500~2500us 对应 0~180°
class Servo final {
public:
    Servo(IPwm& pwm,
          int16_t min_deg = 0, int16_t max_deg = 180,
          uint16_t min_us = 500U, uint16_t max_us = 2500U);

    ErrorCode Init();
    ErrorCode SetAngle(int16_t degree);   // 越界自动限幅

private:
    IPwm& pwm_;
    int16_t min_deg_;
    int16_t max_deg_;
    uint16_t min_us_;
    uint16_t max_us_;
};
