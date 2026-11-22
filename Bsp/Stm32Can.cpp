#include "Stm32Can.hpp"
#include <cstring>

namespace {
Stm32Can* g_can = nullptr;
}  // namespace

Stm32Can::Stm32Can(CAN_HandleTypeDef* hcan) : hcan_(hcan), rx_cb_(nullptr), rx_ctx_(nullptr) {
    if (g_can == nullptr) {
        g_can = this;
    }
}

Stm32Can::~Stm32Can() {
    if (g_can == this) {
        g_can = nullptr;
    }
}

ErrorCode Stm32Can::Init() {
    if (hcan_ == nullptr) {
        return ErrorCode::ERR_INVALID_PARAM;
    }

    // 滤波器 bank0，32 位掩码全 0 —— 接收总线上所有帧
    CAN_FilterTypeDef filter;
    std::memset(&filter, 0, sizeof(filter));
    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(hcan_, &filter) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    if (HAL_CAN_Start(hcan_) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    if (HAL_CAN_ActivateNotification(hcan_, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    return ErrorCode::OK;
}

// 方式一：阻塞发送，邮箱满了就等，超时返回 ERR_TIMEOUT
ErrorCode Stm32Can::Transmit(const CanFrame& frame, uint32_t timeout_ms) {
    if (hcan_ == nullptr) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    if (frame.dlc > 8U) {
        return ErrorCode::ERR_INVALID_PARAM;
    }

    const uint32_t start = HAL_GetTick();
    while (HAL_CAN_GetTxMailboxesFreeLevel(hcan_) == 0U) {
        if ((HAL_GetTick() - start) >= timeout_ms) {
            return ErrorCode::ERR_TIMEOUT;
        }
    }

    CAN_TxHeaderTypeDef tx;
    std::memset(&tx, 0, sizeof(tx));
    tx.StdId = frame.extended ? 0U : frame.id;
    tx.ExtId = frame.extended ? frame.id : 0U;
    tx.IDE = frame.extended ? CAN_ID_EXT : CAN_ID_STD;
    tx.RTR = CAN_RTR_DATA;
    tx.DLC = frame.dlc;
    tx.TransmitGlobalTime = DISABLE;

    uint8_t payload[8] = {0};
    std::memcpy(payload, frame.data, frame.dlc);

    uint32_t mailbox = 0;
    if (HAL_CAN_AddTxMessage(hcan_, &tx, payload, &mailbox) != HAL_OK) {
        return ErrorCode::ERR_BUSY;
    }
    return ErrorCode::OK;
}

// 方式二：中断接收（FIFO0 有报文时进 NotifyRxPending）
ErrorCode Stm32Can::StartReceive(RxCallback cb, void* ctx) {
    if (hcan_ == nullptr || cb == nullptr) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    rx_cb_ = cb;
    rx_ctx_ = ctx;
    return ErrorCode::OK;
}

void Stm32Can::NotifyRxPending(CAN_HandleTypeDef* hcan) {
    if (g_can == nullptr || g_can->hcan_ != hcan || g_can->rx_cb_ == nullptr) {
        return;
    }

    CAN_RxHeaderTypeDef rx;
    uint8_t payload[8] = {0};
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx, payload) != HAL_OK) {
        return;
    }

    CanFrame frame;
    frame.extended = (rx.IDE == CAN_ID_EXT);
    frame.id = frame.extended ? rx.ExtId : rx.StdId;
    frame.dlc = static_cast<uint8_t>(rx.DLC);
    std::memcpy(frame.data, payload, frame.dlc);

    g_can->rx_cb_(g_can->rx_ctx_, frame);
}

extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan) {
    Stm32Can::NotifyRxPending(hcan);
}
