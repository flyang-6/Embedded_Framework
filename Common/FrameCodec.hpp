#pragma once
#include <cstdint>
#include "ErrorCode.hpp"

// 串口帧格式：AA 55 LEN PAYLOAD... CRC8(LEN..PAYLOAD)
constexpr uint8_t kFrameHead0 = 0xAAU;
constexpr uint8_t kFrameHead1 = 0x55U;
constexpr uint8_t kFrameMaxPayload = 64U;

struct Frame {
    uint8_t len;
    uint8_t payload[kFrameMaxPayload];
};

// CRC-8/ATM，poly 0x07，初值 0x00
inline uint8_t Crc8(const uint8_t* data, uint16_t len) {
    uint8_t crc = 0x00U;
    for (uint16_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            if ((crc & 0x80U) != 0U) {
                crc = static_cast<uint8_t>((crc << 1) ^ 0x07U);
            } else {
                crc = static_cast<uint8_t>(crc << 1);
            }
        }
    }
    return crc;
}

// 编码，返回写入字节数；0 表示失败（缓冲区不够 / payload 超长）
inline uint16_t EncodeFrame(const Frame& frame, uint8_t* out, uint16_t out_size) {
    if (out == nullptr || frame.len > kFrameMaxPayload) {
        return 0U;
    }
    const uint16_t total = static_cast<uint16_t>(frame.len + 4U);
    if (out_size < total) {
        return 0U;
    }
    out[0] = kFrameHead0;
    out[1] = kFrameHead1;
    out[2] = frame.len;
    for (uint8_t i = 0; i < frame.len; ++i) {
        out[3U + i] = frame.payload[i];
    }
    out[3U + frame.len] = Crc8(&out[2], static_cast<uint16_t>(frame.len + 1U));
    return total;
}

// 解码；帧头/长度不对返回 ERR_INVALID_PARAM，CRC 不对返回 ERR_CHECKSUM_FAIL
inline ErrorCode DecodeFrame(const uint8_t* buf, uint16_t size, Frame& out) {
    if (buf == nullptr || size < 4U) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    if (buf[0] != kFrameHead0 || buf[1] != kFrameHead1) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    const uint8_t len = buf[2];
    if (len > kFrameMaxPayload || size < static_cast<uint16_t>(len + 4U)) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    if (Crc8(&buf[2], static_cast<uint16_t>(len + 1U)) != buf[3U + len]) {
        return ErrorCode::ERR_CHECKSUM_FAIL;
    }
    out.len = len;
    for (uint8_t i = 0; i < len; ++i) {
        out.payload[i] = buf[3U + i];
    }
    return ErrorCode::OK;
}