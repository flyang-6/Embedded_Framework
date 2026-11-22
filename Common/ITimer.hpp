#pragma once
#include <cstdint>
#include "ErrorCode.hpp"

// 周期定时器接口（中断上下文回调）
class ITimer {
public:
    using Callback = void (*)(void* ctx);

    virtual ~ITimer() = default;
    virtual ErrorCode Start(uint32_t period_ms, Callback cb, void* ctx) = 0;
    virtual ErrorCode Stop() = 0;
};
