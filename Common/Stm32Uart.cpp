#include "Stm32Uart.hpp"

Stm32Uart::Stm32Uart(UART_HandleTypeDef* huart) : huart_(huart) {}

ErrorCode Stm32Uart::Init(uint32_t baudrate) {
    if (huart_ == nullptr) return ErrorCode::ERR_INVALID_PARAM;
    huart_->Init.BaudRate = baudrate;
    if (HAL_UART_Init(huart_) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    return ErrorCode::OK;
}

ErrorCode Stm32Uart::Transmit(const uint8_t* data, uint16_t size) {
    if (huart_ == nullptr || data == nullptr) return ErrorCode::ERR_INVALID_PARAM;

    //
    if (HAL_UART_Transmit(huart_, (uint8_t*)data, size, 100) != HAL_OK) {
        return ErrorCode::ERR_TIMEOUT;
    }
    return ErrorCode::OK;
}

ErrorCode Stm32Uart::Receive(uint8_t* buffer, uint16_t size, uint32_t timeout_ms) {
    if (huart_ == nullptr || buffer == nullptr) return ErrorCode::ERR_INVALID_PARAM;

    if (HAL_UART_Receive(huart_, buffer, size, timeout_ms) != HAL_OK) {
        return ErrorCode::ERR_TIMEOUT;
    }
    return ErrorCode::OK;
}

//
ErrorCode Stm32Uart::StartDmaReceive(uint8_t* buffer, uint16_t size) {
    if (huart_ == nullptr || buffer ==nullptr) return ErrorCode::ERR_INVALID_PARAM;

    if (HAL_UART_Receive_DMA(huart_, buffer, size) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    return ErrorCode::OK;
}