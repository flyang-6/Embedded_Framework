#pragma once
#include "ITimer.hpp"
#include "tim.h"

class Stm32Timer final : public ITimer {
public:
    explicit Stm32Timer(TIM_HandleTypeDef* htim);

    TIM_HandleTypeDef* Handle() const { return htim_; }

    ErrorCode Start(uint32_t period_ms, Callback cb, void* ctx) override;
    ErrorCode Stop() override;

    static void NotifyElapsed(TIM_HandleTypeDef* htim);

private:
    TIM_HandleTypeDef* htim_;
    Callback cb_;
    void* ctx_;
};
