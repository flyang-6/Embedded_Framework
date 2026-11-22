#include "Servo.hpp"

Servo::Servo(IPwm& pwm, int16_t min_deg, int16_t max_deg,
             uint16_t min_us, uint16_t max_us)
    : pwm_(pwm),
      min_deg_(min_deg),
      max_deg_(max_deg),
      min_us_(min_us),
      max_us_(max_us) {}

ErrorCode Servo::Init() {
    return pwm_.Init(50U);   // 舵机标准 50Hz（20ms）
}

ErrorCode Servo::SetAngle(int16_t degree) {
    if (max_deg_ <= min_deg_ || max_us_ <= min_us_) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    if (degree < min_deg_) {
        degree = min_deg_;
    }
    if (degree > max_deg_) {
        degree = max_deg_;
    }

    const int32_t span_us = static_cast<int32_t>(max_us_) - static_cast<int32_t>(min_us_);
    const int32_t span_deg = static_cast<int32_t>(max_deg_) - static_cast<int32_t>(min_deg_);
    const int32_t offset = (static_cast<int32_t>(degree) - min_deg_) * span_us / span_deg;
    const uint16_t pulse = static_cast<uint16_t>(static_cast<int32_t>(min_us_) + offset);

    return pwm_.SetPulseUs(pulse);
}
