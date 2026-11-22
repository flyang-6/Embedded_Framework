#pragma once
#include <cstdint>
#include "main.h"

// 板载 LED，业务层通过它控制灯，不再直接写 HAL_GPIO_xxx
class BoardLed {
public:
    BoardLed(GPIO_TypeDef* port, uint16_t pin, bool active_low);
    void On();
    void Off();
    void Toggle();

private:
    GPIO_TypeDef* port_;
    uint16_t pin_;
    bool active_low_;
};
