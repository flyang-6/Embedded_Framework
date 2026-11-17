#include "Stm32I2c.hpp"

Stm32I2c::Stm32I2c(I2C_HandleTypeDef* hi2c) : hi2c_(hi2c) {
  //  if (hi2c_ == nullptr) return ErrorCode::ERR_INVALID_PARAM;
   // if(HAL_I2C_Init(hi2c_) != HAL_OK) {
  //      return ErrorCode::ERR_INIT_FAIL;
  //  }
  //  return ErrorCode::OK;
}

ErrorCode Stm32I2c::WriteReg(uint8_t dev_addr, uint8_t reg_addr, const uint8_t* data, uint16_t size) {
    if (hi2c_ == nullptr || data == nullptr) return ErrorCode::ERR_INVALID_PARAM;

    //
    if (HAL_I2C_Mem_Write(hi2c_, dev_addr << 1, reg_addr, I2C_MEMADD_SIZE_8BIT, (uint8_t*)data, size, 100) != HAL_OK) {
        return ErrorCode::ERR_TIMEOUT;
    }
    return ErrorCode::OK;
}

ErrorCode Stm32I2c::ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint16_t size) {
    if (hi2c_ == nullptr || data == nullptr) return ErrorCode::ERR_INVALID_PARAM;

    //
    if (HAL_I2C_Mem_Read(hi2c_, dev_addr << 1, reg_addr, I2C_MEMADD_SIZE_8BIT, data, size, 100) != HAL_OK) {
        return ErrorCode::ERR_TIMEOUT;
    }
    return ErrorCode::OK;
}