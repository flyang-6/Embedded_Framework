#include "BoardLed.hpp"

BoardLed::BoardLed(GPIO_TypeDef* port, uint16_t pin, bool active_low)
    : port_(port), pin_(pin), active_low_(active_low) {
    Off();
}

void BoardLed::On() {
    HAL_GPIO_WritePin(port_, pin_, active_low_ ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void BoardLed::Off() {
    HAL_GPIO_WritePin(port_, pin_, active_low_ ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void BoardLed::Toggle() {
    HAL_GPIO_TogglePin(port_, pin_);
}
