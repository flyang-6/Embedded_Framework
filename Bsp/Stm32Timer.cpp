#include "Stm32Timer.hpp"

namespace {
constexpr int kMaxTimers = 2;
Stm32Timer* g_timers[kMaxTimers] = {nullptr, nullptr};

// 本工程用的都是 APB1 定时器；APB1 分频不为 1 时定时器时钟 = PCLK1 * 2
uint32_t TimerClockHz() {
    const uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    const uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> 8;
    return (ppre1 >= 4U) ? (pclk1 * 2U) : pclk1;
}
}  // namespace

Stm32Timer::Stm32Timer(TIM_HandleTypeDef* htim) : htim_(htim), cb_(nullptr), ctx_(nullptr) {
    for (Stm32Timer*& slot : g_timers) {
        if (slot == nullptr) {
            slot = this;
            break;
        }
    }
}

ErrorCode Stm32Timer::Start(uint32_t period_ms, Callback cb, void* ctx) {
    if (htim_ == nullptr) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    // TIM3 是 16 位；按 0.1ms 计数，最大周期 6553ms
    if (period_ms == 0U || period_ms > 6553U) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    const uint32_t tim_clk = TimerClockHz();
    const uint32_t psc = tim_clk / 10000U;   // 计数器 10kHz
    if (psc == 0U) {
        return ErrorCode::ERR_INIT_FAIL;
    }

    __HAL_TIM_SET_PRESCALER(htim_, psc - 1U);
    __HAL_TIM_SET_AUTORELOAD(htim_, period_ms * 10U - 1U);
    __HAL_TIM_SET_COUNTER(htim_, 0U);

    htim_->Instance->EGR = TIM_EGR_UG;              // 立即加载新的 PSC
    __HAL_TIM_CLEAR_FLAG(htim_, TIM_FLAG_UPDATE);   // 清掉 UG 产生的更新标志

    cb_ = cb;
    ctx_ = ctx;

    if (HAL_TIM_Base_Start_IT(htim_) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    return ErrorCode::OK;
}

ErrorCode Stm32Timer::Stop() {
    if (htim_ == nullptr) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    (void)HAL_TIM_Base_Stop_IT(htim_);
    return ErrorCode::OK;
}

void Stm32Timer::NotifyElapsed(TIM_HandleTypeDef* htim) {
    for (Stm32Timer* t : g_timers) {
        if (t != nullptr && t->htim_ == htim && t->cb_ != nullptr) {
            t->cb_(t->ctx_);
            return;
        }
    }
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    Stm32Timer::NotifyElapsed(htim);
}
