#pragma once

// 使用 enum class 强类型枚举 （现代 c++ 做法）
enum class ErrorCode {
    OK = 0,                 //成功
    ERR_TIMEOUT,            //通信超时
    ERR_INIT_FAIL,          //初始化失败
    ERR_INVALID_PARAM,      //参数错误 （比如空指针）
    ERR_CHECKSUM_FAIL,      //数据校验失败
    ERR_BUSY,               //设备忙
    ERR_UNKNOWN             //未知错误
};

// 辅助函数，把错误码转成字符串方便打印
inline const char* ErrorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::OK: return "OK";
        case ErrorCode::ERR_TIMEOUT: return "TIMEOUT";
        case ErrorCode::ERR_INIT_FAIL: return "INIT_FAIL";
        case ErrorCode::ERR_INVALID_PARAM: return "INVALID_PARAM";
        case ErrorCode::ERR_CHECKSUM_FAIL: return "CHECKSUM_FAIL";
        case ErrorCode::ERR_BUSY: return "BUSY";
        default: return "UNKNOWN";
    }
}