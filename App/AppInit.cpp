// 组合根（Composition Root）：
// 全工程只有这个文件出现 huart1 / hi2c1 / hcan1 / htim3 这些 CubeMX 全局句柄。
// 换芯片时只改这里，其余代码一行不动。
#include "main.h"
#include "usart.h"
#include "i2c.h"

#if defined(__has_include)
#  if __has_include("can.h")
#    include "can.h"
#    define APP_HAS_CAN 1
#  endif
#  if __has_include("tim.h")
#    include "tim.h"
#    define APP_HAS_TIM 1
#  endif
#endif

#include "ErrorCode.hpp"
#include "Logger.hpp"
#include "Stm32Uart.hpp"
#include "Stm32I2c.hpp"
#include "BoardLed.hpp"
#include "UartEchoTask.hpp"

#if defined(APP_HAS_CAN)
#  include "Stm32Can.hpp"
#  include "CanEchoTask.hpp"
#endif

#if defined(APP_HAS_TIM)
#  include "Stm32Timer.hpp"
#  include "HeartbeatTask.hpp"
#endif

namespace {
constexpr uint32_t kUartBaudrate = 115200U;
constexpr uint32_t kI2cClockHz = 100000U;
constexpr uint32_t kHeartbeatMs = 1000U;
}  // namespace

// 返回 0 表示成功，否则为失败的错误码（失败原因已由 Logger 打印）
extern "C" int App_Init(void) {
    static uint8_t uart_rx_buf[64];

    static Stm32Uart uart1(&huart1, uart_rx_buf, sizeof(uart_rx_buf));
    static Stm32I2c i2c1(&hi2c1);
    static BoardLed led(GPIOC, GPIO_PIN_13, true);
    static UartEchoTask uart_task(uart1, led);

    ErrorCode rc = uart1.Init(kUartBaudrate);
    if (rc != ErrorCode::OK) {
        return static_cast<int>(rc);   // 串口都没起来，后面没法打日志
    }
    Logger::Init(&uart1);
    Logger::Info("APP", "framework boot");

    rc = i2c1.Init(kI2cClockHz);
    if (rc != ErrorCode::OK) {
        Logger::Error("APP", rc, "i2c init failed");
        return static_cast<int>(rc);
    }
    Logger::Info("APP", "i2c ready (%luHz)", static_cast<unsigned long>(kI2cClockHz));

    rc = uart_task.Start();
    if (rc != ErrorCode::OK) {
        Logger::Error("APP", rc, "uart task start failed");
        return static_cast<int>(rc);
    }

#if defined(APP_HAS_CAN)
    {
        static Stm32Can can1(&hcan1);
        static CanEchoTask can_task(can1, led);

        rc = can1.Init();
        if (rc != ErrorCode::OK) {
            Logger::Error("APP", rc, "can init failed");
            return static_cast<int>(rc);
        }
        rc = can_task.Start();
        if (rc != ErrorCode::OK) {
            Logger::Error("APP", rc, "can task start failed");
            return static_cast<int>(rc);
        }
        Logger::Info("APP", "can ready");
    }
#else
    Logger::Info("APP", "can not configured in CubeMX, skipped");
#endif

#if defined(APP_HAS_TIM)
    {
        static Stm32Timer tim3(&htim3);
        static HeartbeatTask heartbeat(tim3, led, kHeartbeatMs);

        rc = heartbeat.Start();
        if (rc != ErrorCode::OK) {
            Logger::Error("APP", rc, "heartbeat task start failed");
            return static_cast<int>(rc);
        }
        Logger::Info("APP", "timer ready (%lums)", static_cast<unsigned long>(kHeartbeatMs));
    }
#else
    Logger::Info("APP", "tim not configured in CubeMX, heartbeat disabled");
#endif

    Logger::Info("APP", "framework ready");
    return 0;
}
