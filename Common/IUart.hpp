#pragma once
#include <cstdint>
#include "ErrorCode.hpp"

//
class  IUart {
public:
    virtual ~IUart() = default;

    //
    virtual ErrorCode Init(uint32_t baudrate) = 0;

    //
    virtual ErrorCode Transmit(const uint8_t* data, uint16_t size) = 0;

    //
    virtual ErrorCode Receive(uint8_t* buffer, uint16_t size, uint32_t timeout_ms) = 0;
};