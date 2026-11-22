#pragma once
#include <cstdint>
#include "ErrorCode.hpp"

// PWM 输出抽象：舵机、直流电机调速等共用
class IPwm {
public:
    virtual ~IPwm() = default;

    // 设定频率（Hz），实现内部固定 1us 分辨率
    virtual ErrorCode Init(uint32_t freq_hz) = 0;

    // 占空比千分比 0..1000
    virtual ErrorCode SetDuty(uint16_t per_mille) = 0;

    // 直接给高电平脉宽（微秒），舵机这类器件用它更直观
    virtual ErrorCode SetPulseUs(uint16_t pulse_us) = 0;
};
