#pragma once
#include <cstdint>
#include "ErrorCode.hpp"

//
struct CanFrame {
    uint32_t id;
    uint8_t data[8];
    uint8_t dlc;
    bool is_extended;
};

class ICan {
public:
    virtual ~ICan() = default;
    //
    virtual ErrorCode Init(uint32_t baudrate) = 0;
    //
    virtual ErrorCode Transmit(const CanFrame& frame) = 0;
    //
    //
    virtual void RegisterReceiveCallback(void (*callback)(const CanFrame& frame)) = 0;    
};