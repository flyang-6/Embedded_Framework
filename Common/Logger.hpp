#pragma once
#include <cstdint>
#include "ErrorCode.hpp"

class IUart;

// 统一日志：只依赖 IUart 接口，不碰 HAL，也不依赖 printf 重定向
class Logger {
public:
    static void Init(IUart* port);

    static void Info(const char* tag, const char* fmt, ...);

    // 带错误码的错误日志，格式：[ERROR][tag] msg (code=TIMEOUT)
    static void Error(const char* tag, ErrorCode code, const char* fmt, ...);
};
