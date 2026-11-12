#pragma once
#include "IUart.hpp"
#include "usart.h"

class Stm32Uart : public IUart {
private:
    UART_HandleTypeDef* huart_;

public:
    Stm32Uart(UART_HandleTypeDef* huart);
    ~Stm32Uart() override = default;

    ErrorCode Init(uint32_t baudrate) override;
    ErrorCode Transmit(const uint8_t* data, uint16_t size) override;
    ErrorCode Receive(uint8_t* buffer, uint16_t size, uint32_t timeout_ms) override;

    //
    ErrorCode StartDmaReceive(uint8_t* buffer, uint16_t size);
};

