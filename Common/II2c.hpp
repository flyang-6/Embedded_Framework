#pragma once
#include <cstdint>
#include "ErrorCode.hpp"

// I2C 抽象接口（8 位寄存器地址的常见器件）
class II2c {
public:
    virtual ~II2c() = default;

    virtual ErrorCode Init(uint32_t clock_hz) = 0;

    // 方式一：阻塞读写
    virtual ErrorCode WriteReg(uint8_t dev_addr, uint8_t reg_addr,
                               const uint8_t* data, uint16_t size,
                               uint32_t timeout_ms) = 0;
    virtual ErrorCode ReadReg(uint8_t dev_addr, uint8_t reg_addr,
                              uint8_t* data, uint16_t size,
                              uint32_t timeout_ms) = 0;

    // 方式二：中断读写（非阻塞），发起后调用 WaitAsyncDone 取结果
    virtual ErrorCode WriteRegAsync(uint8_t dev_addr, uint8_t reg_addr,
                                    const uint8_t* data, uint16_t size) = 0;
    virtual ErrorCode ReadRegAsync(uint8_t dev_addr, uint8_t reg_addr,
                                   uint8_t* data, uint16_t size) = 0;
    virtual ErrorCode WaitAsyncDone(uint32_t timeout_ms) = 0;
};
