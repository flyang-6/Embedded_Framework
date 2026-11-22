#pragma once
#include <cstdint>

// 统一错误码：所有对外接口都必须返回它，禁止静默失败
enum class ErrorCode : uint32_t {
    OK = 0,
    ERR_INVALID_PARAM  = 1,   // 空指针 / 长度非法 / 参数越界
    ERR_NOT_INIT       = 2,   // 未初始化就调用
    ERR_TIMEOUT        = 3,   // 通信超时
    ERR_BUSY           = 4,   // 外设忙
    ERR_INIT_FAIL      = 5,   // 初始化失败
    ERR_CHECKSUM_FAIL  = 6,   // 数据校验失败
    ERR_NO_MEM         = 7,   // 队列/任务创建失败
    ERR_HW             = 8,   // 硬件错误（HAL 返回错误但非超时）
    ERR_UNSUPPORTED    = 9,   // 当前配置不支持
    ERR_UNKNOWN        = 10,
};

inline const char* ErrorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::OK:                return "OK";
        case ErrorCode::ERR_INVALID_PARAM: return "INVALID_PARAM";
        case ErrorCode::ERR_NOT_INIT:      return "NOT_INIT";
        case ErrorCode::ERR_TIMEOUT:       return "TIMEOUT";
        case ErrorCode::ERR_BUSY:          return "BUSY";
        case ErrorCode::ERR_INIT_FAIL:     return "INIT_FAIL";
        case ErrorCode::ERR_CHECKSUM_FAIL: return "CHECKSUM_FAIL";
        case ErrorCode::ERR_NO_MEM:        return "NO_MEM";
        case ErrorCode::ERR_HW:            return "HW_ERROR";
        case ErrorCode::ERR_UNSUPPORTED:   return "UNSUPPORTED";
        default:                           return "UNKNOWN";
    }
}
