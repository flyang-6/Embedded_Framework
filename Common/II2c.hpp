#pragma once
#include <cstdint>
#include "ErrorCode.hpp"

class II2c {
public:
    virtual ~II2c() = default;
    //
    virtual ErrorCode Init() = 0;
    //
    virtual ErrorCode WriteReg(uint8_t dev_addr, uint8_t reg_addr, const uint8_t* data, uint16_t size) = 0;
    //
    virtual ErrorCode ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint16_t size) = 0;
};