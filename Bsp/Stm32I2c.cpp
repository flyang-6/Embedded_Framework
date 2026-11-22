#include "Stm32I2c.hpp"

namespace {
Stm32I2c* g_i2c = nullptr;

ErrorCode MapStatus(HAL_StatusTypeDef st) {
    switch (st) {
        case HAL_OK:      return ErrorCode::OK;
        case HAL_TIMEOUT: return ErrorCode::ERR_TIMEOUT;
        case HAL_BUSY:    return ErrorCode::ERR_BUSY;
        default:          return ErrorCode::ERR_HW;
    }
}
}  // namespace

Stm32I2c::Stm32I2c(I2C_HandleTypeDef* hi2c) : hi2c_(hi2c), done_(nullptr), async_ok_(0) {
    done_ = xSemaphoreCreateBinary();
    if (g_i2c == nullptr) {
        g_i2c = this;
    }
}

Stm32I2c::~Stm32I2c() {
    if (g_i2c == this) {
        g_i2c = nullptr;
    }
    if (done_ != nullptr) {
        vSemaphoreDelete(done_);
        done_ = nullptr;
    }
}

// 原来缺失的 Init 实现（旧代码只有声明，一调用就链接失败）
ErrorCode Stm32I2c::Init(uint32_t clock_hz) {
    if (hi2c_ == nullptr || done_ == nullptr) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    if (clock_hz == 0U) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    hi2c_->Init.ClockSpeed = clock_hz;
    if (HAL_I2C_Init(hi2c_) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    return ErrorCode::OK;
}

// 方式一：阻塞
ErrorCode Stm32I2c::WriteReg(uint8_t dev_addr, uint8_t reg_addr, const uint8_t* data,
                             uint16_t size, uint32_t timeout_ms) {
    if (hi2c_ == nullptr || data == nullptr || size == 0U) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    return MapStatus(HAL_I2C_Mem_Write(hi2c_, static_cast<uint16_t>(dev_addr << 1), reg_addr,
                                       I2C_MEMADD_SIZE_8BIT, const_cast<uint8_t*>(data),
                                       size, timeout_ms));
}

ErrorCode Stm32I2c::ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data,
                            uint16_t size, uint32_t timeout_ms) {
    if (hi2c_ == nullptr || data == nullptr || size == 0U) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    return MapStatus(HAL_I2C_Mem_Read(hi2c_, static_cast<uint16_t>(dev_addr << 1), reg_addr,
                                      I2C_MEMADD_SIZE_8BIT, data, size, timeout_ms));
}

// 方式二：中断（非阻塞）
ErrorCode Stm32I2c::WriteRegAsync(uint8_t dev_addr, uint8_t reg_addr,
                                  const uint8_t* data, uint16_t size) {
    if (hi2c_ == nullptr || data == nullptr || size == 0U) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    async_ok_ = 0;
    (void)xSemaphoreTake(done_, 0);   // 清掉上一次残留

    return MapStatus(HAL_I2C_Mem_Write_IT(hi2c_, static_cast<uint16_t>(dev_addr << 1), reg_addr,
                                          I2C_MEMADD_SIZE_8BIT, const_cast<uint8_t*>(data), size));
}

ErrorCode Stm32I2c::ReadRegAsync(uint8_t dev_addr, uint8_t reg_addr,
                                 uint8_t* data, uint16_t size) {
    if (hi2c_ == nullptr || data == nullptr || size == 0U) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    async_ok_ = 0;
    (void)xSemaphoreTake(done_, 0);

    return MapStatus(HAL_I2C_Mem_Read_IT(hi2c_, static_cast<uint16_t>(dev_addr << 1), reg_addr,
                                         I2C_MEMADD_SIZE_8BIT, data, size));
}

ErrorCode Stm32I2c::WaitAsyncDone(uint32_t timeout_ms) {
    if (done_ == nullptr) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    if (xSemaphoreTake(done_, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        return ErrorCode::ERR_TIMEOUT;
    }
    return (async_ok_ != 0U) ? ErrorCode::OK : ErrorCode::ERR_HW;
}

void Stm32I2c::NotifyComplete(I2C_HandleTypeDef* hi2c, bool success) {
    if (g_i2c == nullptr || g_i2c->hi2c_ != hi2c) {
        return;
    }
    g_i2c->async_ok_ = success ? 1U : 0U;

    BaseType_t woken = pdFALSE;
    if (g_i2c->done_ != nullptr) {
        (void)xSemaphoreGiveFromISR(g_i2c->done_, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

extern "C" void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef* hi2c) {
    Stm32I2c::NotifyComplete(hi2c, true);
}

extern "C" void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef* hi2c) {
    Stm32I2c::NotifyComplete(hi2c, true);
}

extern "C" void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c) {
    Stm32I2c::NotifyComplete(hi2c, false);
}
