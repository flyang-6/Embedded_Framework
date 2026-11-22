# 嵌入式软件框架总览 (Framework Overview)

## 1. 目录结构总览 (Directory Structure)
我们的项目采用清晰的分层架构，代码目录结构如下：

```text
My_framework/
├── Common/                 # Module 层：接口与通用组件（零 HAL 依赖）
│   ├── ErrorCode.hpp       # 统一错误码（全局通用）
│   ├── Logger.hpp/.cpp     # 统一日志打印（走 IUart 接口，不依赖 printf）
│   ├── IUart.hpp           # 串口硬件纯虚接口（"合同"）
│   ├── II2c.hpp            # I2C硬件纯虚接口
│   ├── ICan.hpp            # CAN硬件纯虚接口
│   └── ITimer.hpp          # 周期定时器纯虚接口
├── Bsp/                    # BSP 层：唯一允许包含 HAL 头文件的地方
│   ├── Stm32Uart.hpp/.cpp  # 串口实现（阻塞发送 + DMA/空闲中断接收）
│   ├── Stm32I2c.hpp/.cpp   # I2C实现（阻塞 + 中断两种方式）
│   ├── Stm32Can.hpp/.cpp   # CAN实现（阻塞发送 + FIFO0 中断接收）
│   ├── Stm32Timer.hpp/.cpp # 定时器实现（周期中断）
│   └── BoardLed.hpp/.cpp   # 板载 LED，替代业务层直接写 GPIO
├── App/                    # Application 层：只依赖接口
│   ├── AppInit.cpp         # 组合根：装配所有对象、统一初始化
│   ├── UartEchoTask.*      # 串口回显任务（队列）
│   ├── CanEchoTask.*       # CAN 回显任务（队列）
│   └── HeartbeatTask.*     # 心跳任务（二值信号量）
├── Core/                   # STM32CubeMX生成的底层代码（HAL, main.c等）
├── Drivers/                # STM32官方HAL库驱动文件
├── Middlewares/            # FreeRTOS操作系统源码
└── CMakeLists.txt          # 构建脚本
```

> 2026-11-22 的分层改造细节见 `docs/Step2_Layering_Refactor.md`。

+-------------------------------------------------------------+
|                     Application 层 (业务层)                 |
|  负责具体业务逻辑与 FreeRTOS 任务调度 (如：UartEchoTask)        |
+-------------------------------------------------------------+
                              |
                              v (依赖注入 & 接口调用)
+-------------------------------------------------------------+
|                     Module 层 (模块层)                      |
|  定义纯虚接口 (IUart, II2c) 和 通用组件 (Logger, ErrorCode) |
|  【核心原则】：不依赖具体硬件，业务代码绝不直接调用 HAL 库。|
+-------------------------------------------------------------+
                              |
                              v (继承实现)
+-------------------------------------------------------------+
|                     BSP 层 (板级支持层)                     |
|  具体的硬件驱动实现 (如：Stm32Uart, Stm32I2c)               |
|  内部封装了 STM32 HAL 库的 API (如 HAL_UART_Transmit)       |
+-------------------------------------------------------------+
                              |
                              v (直接操作寄存器/句柄)
+-------------------------------------------------------------+
|                     Hardware 层 (硬件层)                    |
|   STM32F103C8T6 芯片、USART1、I2C1 物理外设、DMA控制器      |
+-------------------------------------------------------------+