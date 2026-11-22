#pragma once
#include <cstdint>
#include "ErrorCode.hpp"

struct CanFrame {
    uint32_t id = 0;
    uint8_t  data[8] = {0};
    uint8_t  dlc = 0;
    bool     extended = false;
};

class ICan {
public:
    using RxCallback = void (*)(void* ctx, const CanFrame& frame);

    virtual ~ICan() = default;

    virtual ErrorCode Init() = 0;                                              // 滤波器 + 启动 + 开中断
    virtual ErrorCode Transmit(const CanFrame& frame, uint32_t timeout_ms) = 0; // 方式一：阻塞发送
    virtual ErrorCode StartReceive(RxCallback cb, void* ctx) = 0;               // 方式二：中断接收
};
