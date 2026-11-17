#pragma once
#include "II2c.hpp"
#include "i2c.h"

class Stm32I2c : public II2c {
private:
    I2C_HandleTypeDef* hi2c_;
public:
    Stm32I2c(I2C_HandleTypeDef* hi2c);
    ErrorCode Init() override;
    ErrorCode WriteReg(uint8_t dev_addr, uint8_t reg_addr, const uint8_t* data, uint16_t size) override;
    ErrorCode ReadReg(uint8_t dev_addr, uint8_t reg_addr,uint8_t* data, uint16_t size) override;
};