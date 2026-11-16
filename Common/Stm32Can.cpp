#include "Stm32Can.hpp"
#include "Logger.hpp"

Stm32Can::Stm32Can(can-HandleTypeDef* hcan) : hcan_(hcan) {}

ErrorCode Stm32Can::Init(uint32_t baudrate) {
    if (hcan_ == nullptr) return ErrorCode::ERR_INVALID_PARAM;

    //
    if (HAL_CAN_Start(hcan_) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }

    //
    if (HAL_CAN_ActivateNotification(hcan_, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }

    return ErrorCode::OK;
}

ErrorCode Stm32Can::Transmit(const CanFrame& frame) {
    if (hcan_ == nullptr) return ErrorCode::ERR_INVALID_PARAM;

    CAN_TxHeaderTYpeDef txHeader;
    uint32_t txMailbox;

    //
    txHeader.StdId = frame.id;
    txHeader.ExtId = frame.id;
    txHeader.IDE = frame.is_extended ? CAN_ID_EXT : CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = frame.dlc;
    txHeader.TransmitGlobalTime = DISABLE;

    //
    IF (HAL_CAN_AddTxMessage(hcan_, &txHeader, (uint8_t*)frame.data, &txMailbox) != HAL_OK) {
        return ErrorCode::ERR_TIMROUT:
    }
    return ErrorCode::OK;
}

void Stm32Can::RegisterReceiveCallback(void (*callback)(const CanFrame& frame)) {
    //
    //
}