#pragma once
#include "ICan.hpp"
#include "can.h"

class Stm32Can : public ICan {
public:
    Stm32Can(CAN_HandleTypeDef* hcan);
    ErrorCode Init(uint32_t baaudrate) override;
    ErrorCode Transmit(const CanFrame& frame) override;
    void RegisterReceiveCallback(void (*callback)(const CanFrame& frame)) override;

private:
    CAN_HandleTypeDef* hcan_;
};