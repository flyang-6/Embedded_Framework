#pragma once
#include "II2c.hpp"
#include "i2c.h"

#include "FreeRTOS.h"
#include "semphr.h"

class Stm32I2c final : public II2c {
public:
    explicit Stm32I2c(I2C_HandleTypeDef* hi2c);
    ~Stm32I2c() override;

    I2C_HandleTypeDef* Handle() const { return hi2c_; }

    ErrorCode Init(uint32_t clock_hz) override;
    ErrorCode WriteReg(uint8_t dev_addr, uint8_t reg_addr, const uint8_t* data,
                       uint16_t size, uint32_t timeout_ms) override;
    ErrorCode ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data,
                      uint16_t size, uint32_t timeout_ms) override;
    ErrorCode WriteRegAsync(uint8_t dev_addr, uint8_t reg_addr,
                            const uint8_t* data, uint16_t size) override;
    ErrorCode ReadRegAsync(uint8_t dev_addr, uint8_t reg_addr,
                           uint8_t* data, uint16_t size) override;
    ErrorCode WaitAsyncDone(uint32_t timeout_ms) override;

    static void NotifyComplete(I2C_HandleTypeDef* hi2c, bool success);

private:
    I2C_HandleTypeDef* hi2c_;
    SemaphoreHandle_t done_;
    volatile uint8_t async_ok_;
};
