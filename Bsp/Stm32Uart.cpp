#include "Stm32Uart.hpp"

namespace {
constexpr int kMaxUarts = 2;
Stm32Uart* g_uarts[kMaxUarts] = {nullptr, nullptr};

Stm32Uart* Find(UART_HandleTypeDef* huart) {
    for (Stm32Uart* u : g_uarts) {
        if (u != nullptr && u->Handle() == huart) {
            return u;
        }
    }
    return nullptr;
}
}  // namespace

Stm32Uart::Stm32Uart(UART_HandleTypeDef* huart, uint8_t* rx_buf, uint16_t rx_size)
    : huart_(huart), rx_buf_(rx_buf), rx_size_(rx_size), rx_cb_(nullptr), rx_ctx_(nullptr) {
    for (Stm32Uart*& slot : g_uarts) {
        if (slot == nullptr) {
            slot = this;
            break;
        }
    }
}

Stm32Uart::~Stm32Uart() {
    for (Stm32Uart*& slot : g_uarts) {
        if (slot == this) {
            slot = nullptr;
        }
    }
}

ErrorCode Stm32Uart::Init(uint32_t baudrate) {
    if (huart_ == nullptr || rx_buf_ == nullptr || rx_size_ == 0U) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    huart_->Init.BaudRate = baudrate;
    if (HAL_UART_Init(huart_) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    __HAL_UART_ENABLE_IT(huart_, UART_IT_IDLE);   // 空闲中断：判一帧结束
    __HAL_UART_CLEAR_IDLEFLAG(huart_);
    return ErrorCode::OK;
}

// 方式一：阻塞发送
ErrorCode Stm32Uart::Transmit(const uint8_t* data, uint16_t size, uint32_t timeout_ms) {
    if (huart_ == nullptr || (data == nullptr && size > 0U)) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    if (size == 0U) {
        return ErrorCode::OK;
    }

    HAL_StatusTypeDef st = HAL_UART_Transmit(huart_, const_cast<uint8_t*>(data), size, timeout_ms);
    if (st == HAL_OK) {
        return ErrorCode::OK;
    }
    if (st == HAL_TIMEOUT) {
        return ErrorCode::ERR_TIMEOUT;
    }
    if (st == HAL_BUSY) {
        return ErrorCode::ERR_BUSY;
    }
    return ErrorCode::ERR_HW;
}

// 方式二：DMA + 空闲中断接收
ErrorCode Stm32Uart::StartReceive(RxCallback cb, void* ctx) {
    if (huart_ == nullptr || rx_buf_ == nullptr) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    rx_cb_ = cb;
    rx_ctx_ = ctx;

    __HAL_UART_ENABLE_IT(huart_, UART_IT_IDLE);
    __HAL_UART_ENABLE_IT(huart_, UART_IT_ERR);   // 溢出/帧错误也要上报
    if (HAL_UART_Receive_DMA(huart_, rx_buf_, rx_size_) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    return ErrorCode::OK;
}

ErrorCode Stm32Uart::StopReceive() {
    if (huart_ == nullptr) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    (void)HAL_UART_AbortReceive(huart_);
    __HAL_UART_DISABLE_IT(huart_, UART_IT_IDLE);
    rx_cb_ = nullptr;
    rx_ctx_ = nullptr;
    return ErrorCode::OK;
}

void Stm32Uart::Deliver(uint16_t len) {
    if (rx_cb_ != nullptr && len > 0U) {
        rx_cb_(rx_ctx_, rx_buf_, len);
    }
}

void Stm32Uart::Rearm() {
    (void)HAL_UART_Receive_DMA(huart_, rx_buf_, rx_size_);
}

// 缓冲区收满（DMA 传输完成）
void Stm32Uart::NotifyDmaComplete(UART_HandleTypeDef* huart) {
    Stm32Uart* self = Find(huart);
    if (self == nullptr) {
        return;
    }
    self->Deliver(self->rx_size_);
    self->Rearm();
}

// 空闲中断：一帧结束（不定长帧的关键）
void Stm32Uart::NotifyIdle(UART_HandleTypeDef* huart) {
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) == RESET) {
        return;
    }
    __HAL_UART_CLEAR_IDLEFLAG(huart);

    Stm32Uart* self = Find(huart);
    if (self == nullptr || huart->hdmarx == nullptr) {
        return;
    }

    uint32_t remaining = __HAL_DMA_GET_COUNTER(huart->hdmarx);
    if (remaining > self->rx_size_) {
        return;
    }
    uint16_t received = static_cast<uint16_t>(self->rx_size_ - remaining);
    if (received == 0U) {
        return;   // 只是线路空闲，DMA 继续跑
    }

    (void)HAL_UART_DMAStop(huart);
    self->Deliver(received);
    self->Rearm();
}

// 溢出/帧错误：DMA 会停摆，必须重新武装，禁止静默
void Stm32Uart::NotifyError(UART_HandleTypeDef* huart) {
    Stm32Uart* self = Find(huart);
    if (self == nullptr) {
        return;
    }
    (void)HAL_UART_DMAStop(huart);
    __HAL_UART_CLEAR_OREFLAG(huart);
    if (self->rx_cb_ != nullptr) {
        self->Rearm();
    }
}

// ---- HAL 弱函数重写（C 链接）----
extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
    Stm32Uart::NotifyDmaComplete(huart);
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    Stm32Uart::NotifyError(huart);
}

// ---- 给 C 语言中断服务函数调用 ----
extern "C" void Bsp_Uart_IdleIrq(UART_HandleTypeDef* huart) {
    Stm32Uart::NotifyIdle(huart);
}
