#pragma once
#include "IUart.hpp"
#include "usart.h"          // 只有 BSP 层允许包含 CubeMX 头

class Stm32Uart final : public IUart {
public:
    Stm32Uart(UART_HandleTypeDef* huart, uint8_t* rx_buf, uint16_t rx_size);
    ~Stm32Uart() override;

    UART_HandleTypeDef* Handle() const { return huart_; }

    ErrorCode Init(uint32_t baudrate) override;
    ErrorCode Transmit(const uint8_t* data, uint16_t size, uint32_t timeout_ms) override;
    ErrorCode StartReceive(RxCallback cb, void* ctx) override;
    ErrorCode StopReceive() override;

    // HAL 回调 / 中断入口，框架内部使用
    static void NotifyDmaComplete(UART_HandleTypeDef* huart);
    static void NotifyIdle(UART_HandleTypeDef* huart);
    static void NotifyError(UART_HandleTypeDef* huart);

private:
    void Deliver(uint16_t len);
    void Rearm();

    UART_HandleTypeDef* huart_;
    uint8_t* rx_buf_;
    uint16_t rx_size_;
    RxCallback rx_cb_;
    void* rx_ctx_;
};
