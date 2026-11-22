#pragma once
#include "ICan.hpp"
#include "can.h"

class Stm32Can final : public ICan {
public:
    explicit Stm32Can(CAN_HandleTypeDef* hcan);
    ~Stm32Can() override;

    CAN_HandleTypeDef* Handle() const { return hcan_; }

    ErrorCode Init() override;
    ErrorCode Transmit(const CanFrame& frame, uint32_t timeout_ms) override;
    ErrorCode StartReceive(RxCallback cb, void* ctx) override;

    static void NotifyRxPending(CAN_HandleTypeDef* hcan);

private:
    CAN_HandleTypeDef* hcan_;
    RxCallback rx_cb_;
    void* rx_ctx_;
};
