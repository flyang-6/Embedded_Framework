#include "Stm32Pwm.hpp"

namespace {

void EnableTimerClock(TIM_TypeDef* instance) {
    if (instance == TIM1) {
        __HAL_RCC_TIM1_CLK_ENABLE();
    } else if (instance == TIM2) {
        __HAL_RCC_TIM2_CLK_ENABLE();
    } else if (instance == TIM3) {
        __HAL_RCC_TIM3_CLK_ENABLE();
    } else if (instance == TIM4) {
        __HAL_RCC_TIM4_CLK_ENABLE();
    }
}

void EnablePortClock(GPIO_TypeDef* port) {
    if (port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
}

// 定时器时钟：APB 分频为 1 时等于 PCLK，否则是 PCLK 的两倍
uint32_t TimerClockHz(TIM_TypeDef* instance) {
    if (instance == TIM1) {
        const uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
        const uint32_t ppre2 = (RCC->CFGR & RCC_CFGR_PPRE2) >> 11;
        return (ppre2 >= 4U) ? (pclk2 * 2U) : pclk2;
    }
    const uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    const uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> 8;
    return (ppre1 >= 4U) ? (pclk1 * 2U) : pclk1;
}

}  // namespace

Stm32Pwm::Stm32Pwm(TIM_TypeDef* instance, uint32_t channel,
                   GPIO_TypeDef* port, uint16_t pin)
    : channel_(channel), port_(port), pin_(pin), period_us_(0) {
    htim_.Instance = instance;
    htim_.State = HAL_TIM_STATE_RESET;
}

ErrorCode Stm32Pwm::Init(uint32_t freq_hz) {
    if (htim_.Instance == nullptr || port_ == nullptr ||
        freq_hz == 0U || freq_hz > 1000000U) {
        return ErrorCode::ERR_INVALID_PARAM;
    }

    const uint32_t tim_clk = TimerClockHz(htim_.Instance);
    const uint32_t psc = (tim_clk + 999999U) / 1000000U;   // 计数器 1MHz
    if (psc == 0U) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    const uint32_t ticks = 1000000U / freq_hz;             // 周期（us）
    if (ticks == 0U || ticks > 65536U) {
        return ErrorCode::ERR_INVALID_PARAM;
    }

    EnableTimerClock(htim_.Instance);
    EnablePortClock(port_);

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = pin_;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port_, &gpio);

    htim_.Init.Prescaler = psc - 1U;
    htim_.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_.Init.Period = ticks - 1U;
    htim_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&htim_) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 0U;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim_, &oc, channel_) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }
    if (HAL_TIM_PWM_Start(&htim_, channel_) != HAL_OK) {
        return ErrorCode::ERR_INIT_FAIL;
    }

    period_us_ = static_cast<uint16_t>(ticks);
    return ErrorCode::OK;
}

ErrorCode Stm32Pwm::SetDuty(uint16_t per_mille) {
    if (period_us_ == 0U) {
        return ErrorCode::ERR_NOT_INIT;
    }
    if (per_mille > 1000U) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    const uint32_t ccr = (static_cast<uint32_t>(period_us_) * per_mille) / 1000U;
    __HAL_TIM_SET_COMPARE(&htim_, channel_, ccr);
    return ErrorCode::OK;
}

ErrorCode Stm32Pwm::SetPulseUs(uint16_t pulse_us) {
    if (period_us_ == 0U) {
        return ErrorCode::ERR_NOT_INIT;
    }
    if (pulse_us > period_us_) {
        return ErrorCode::ERR_INVALID_PARAM;
    }
    __HAL_TIM_SET_COMPARE(&htim_, channel_, pulse_us);
    return ErrorCode::OK;
}
