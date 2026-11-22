#pragma once
#include <cstdint>
#include "ErrorCode.hpp"

// 串口抽象接口（Module 层，不含任何 HAL 类型）
class IUart {
public:
    // 收到一帧数据时的回调；运行在中断上下文，禁止阻塞、禁止打日志
    using RxCallback = void (*)(void* ctx, const uint8_t* data, uint16_t len);

    virtual ~IUart() = default;

    virtual ErrorCode Init(uint32_t baudrate) = 0;

    // 方式一：阻塞发送，超时返回 ERR_TIMEOUT
    virtual ErrorCode Transmit(const uint8_t* data, uint16_t size, uint32_t timeout_ms) = 0;

    // 方式二：DMA + 空闲中断接收，按“不定长帧”回调
    virtual ErrorCode StartReceive(RxCallback cb, void* ctx) = 0;
    virtual ErrorCode StopReceive() = 0;
};
