#pragma once
#include "IPwm.hpp"
#include "tim.h"

// BSP 层自己完成 TIMx 的时钟、GPIO、时基和通道配置，
// 因此这个 PWM 通道不依赖 CubeMX 是否配置过该定时器（移植时也少一步）。
class Stm32Pwm final : public IPwm {
public:
    Stm32Pwm(TIM_TypeDef* instance, uint32_t channel, GPIO_TypeDef* port, uint16_t pin);

    ErrorCode Init(uint32_t freq_hz) override;
    ErrorCode SetDuty(uint16_t per_mille) override;
    ErrorCode SetPulseUs(uint16_t pulse_us) override;

private:
    TIM_HandleTypeDef htim_;
    uint32_t channel_;
    GPIO_TypeDef* port_;
    uint16_t pin_;
    uint16_t period_us_;   // 一个周期多少微秒，Init 后有效
};
