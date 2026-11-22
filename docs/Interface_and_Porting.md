# 接口说明与移植指南

> 配套文档：`Architecture.md`（架构图 / 目录说明）、`Step2_Layering_Refactor.md`（重构记录 / 踩坑）。

---

## 1. 分层依赖规则

| 层 | 目录 | 允许包含 | 禁止 |
|---|---|---|---|
| Application | `App/` | `Common/` 的接口、`Bsp/` 的构造入口 | 任何 `HAL_xxx`、`stm32f1xx_hal.h` |
| Module | `Common/` | 标准库、FreeRTOS 同步原语 | 任何 HAL 头、任何 CubeMX 生成头 |
| BSP | `Bsp/` | `usart.h` / `i2c.h` / `can.h` / `tim.h`、HAL | 业务逻辑 |

**唯一例外**：`App/AppInit.cpp` 是组合根，全工程只有它 `#include` 了 CubeMX 生成的外设头，
并把 `huart1 / hi2c1 / hcan / htim3` 交给 BSP 层。换芯片时只改这一个文件。

---

## 2. 接口说明

### 2.1 `IUart`（`Common/IUart.hpp`）

| 方法 | 参数 | 返回 | 说明 |
|---|---|---|---|
| `Init(baudrate)` | 波特率 | `OK` / `ERR_INVALID_PARAM` / `ERR_INIT_FAIL` | 重新配置并启动串口 |
| `Transmit(data, size, timeout_ms)` | 数据、长度、超时 | `OK` / `ERR_TIMEOUT` / `ERR_BUSY` / `ERR_HW` / `ERR_INVALID_PARAM` | **阻塞**发送 |
| `StartReceive(cb, ctx)` | 回调、上下文 | `OK` / `ERR_INIT_FAIL` | 启动 **DMA + 空闲中断**接收，收到一帧回调一次 |
| `StopReceive()` | — | `OK` | 停止接收并解绑回调 |

回调原型：`void (*)(void* ctx, const uint8_t* data, uint16_t len)`
**运行在中断上下文**：只能做 `xQueueSendFromISR` 这类动作，禁止阻塞、禁止 `Logger`。

### 2.2 `II2c`（`Common/II2c.hpp`）

| 方法 | 方式 | 返回 | 说明 |
|---|---|---|---|
| `Init(clock_hz)` | — | `OK` / `ERR_INIT_FAIL` | 设置总线速率（100k / 400k） |
| `WriteReg(dev, reg, data, size, timeout_ms)` | 阻塞 | 同 UART | 写寄存器 |
| `ReadReg(dev, reg, data, size, timeout_ms)` | 阻塞 | 同 UART | 读寄存器 |
| `WriteRegAsync(...)` / `ReadRegAsync(...)` | 中断 | `OK` / `ERR_BUSY` / `ERR_HW` | 发起传输后立即返回 |
| `WaitAsyncDone(timeout_ms)` | — | `OK` / `ERR_TIMEOUT` / `ERR_HW` | 等中断传输结束（任务上下文调用） |

设备地址传 **7 位地址**，驱动内部左移一位。

### 2.3 `ICan`（`Common/ICan.hpp`）

| 方法 | 方式 | 返回 | 说明 |
|---|---|---|---|
| `Init()` | — | `OK` / `ERR_INIT_FAIL` | 配滤波器（全通）+ 启动 + 打开 FIFO0 中断 |
| `Transmit(frame, timeout_ms)` | 阻塞 | `OK` / `ERR_TIMEOUT` / `ERR_BUSY` / `ERR_INVALID_PARAM` | 邮箱满时等待，超时返回错误 |
| `StartReceive(cb, ctx)` | 中断 | `OK` / `ERR_INVALID_PARAM` | 收到帧回调 |

`CanFrame`：`id`、`data[8]`、`dlc`（≤8）、`extended`（标准帧 / 扩展帧）。

### 2.4 `ITimer`（`Common/ITimer.hpp`）

| 方法 | 参数 | 返回 | 说明 |
|---|---|---|---|
| `Start(period_ms, cb, ctx)` | 周期、回调 | `OK` / `ERR_INVALID_PARAM`（0 或 >6553ms）/ `ERR_INIT_FAIL` | 周期中断，驱动内部按 PCLK1 自动算预分频 |
| `Stop()` | — | `OK` | 停止 |

回调同样运行在中断上下文，典型用法是喂一个二值信号量给任务。

---

## 3. 三种通信方式与适用场景

| 外设 | 阻塞 | 中断 | DMA | 本工程采用 | 选型理由 |
|---|---|---|---|---|---|
| UART | ✅ `Transmit` | — | ✅ `StartReceive`（DMA+IDLE） | 阻塞发送 + DMA 接收 | 发送多是短帧、调用点简单；接收长度不定，DMA+IDLE 让 CPU 零等待且能判帧边界 |
| I2C | ✅ `WriteReg/ReadReg` | ✅ `*Async + WaitAsyncDone` | — | 阻塞 + 中断 | 上电配置类短事务用阻塞最直观；周期性采样用中断，任务阻塞在信号量上不占 CPU |
| CAN | ✅ `Transmit` | ✅ `StartReceive` | — | 阻塞发送 + 中断接收 | 总线异步，接收必须中断；发送只在邮箱满时短暂等待 |
| TIM | — | ✅ | — | 中断 | 只做节拍，ISR 里只发信号量 |

**错误码约定**：`ERR_TIMEOUT` 只表示"等超时"，`ERR_HW` 表示 HAL 返回错误，`ERR_BUSY` 表示外设正被占用，
`ERR_INVALID_PARAM` 表示空指针/长度非法。任何对外接口都不允许"失败但不返回错误码"。

---

## 4. 移植到另一块 STM32（最小验证：串口 + 一个定时器）

### 4.1 目标

保留 `Common/ Bsp/ App/` 三个目录不动，只替换 CubeMX 生成的底层代码和 `AppInit.cpp` 里的句柄。

### 4.2 步骤

1. **新建 CubeMX 工程**：选目标型号（如 STM32F411CEU6 / STM32G431），配好时钟树
2. **配串口**：任意 USARTx → 异步模式 → 加 DMA（RX，Normal）→ 打开全局中断
3. **配定时器**：任意 TIMx → Internal Clock → 打开全局中断（PSC/ARR 随便填，`Stm32Timer::Start` 会重算）
4. **（可选）配 I2C / CAN**：同样打开对应中断；CAN 若只做自测，`Mode` 选 Loopback（CubeMX 没有就手改 `can.c`）
5. **Toolchain 选 CMake**，生成代码
6. **替换底层**：把新工程的 `Core/`、`Drivers/`、`Middlewares/`、`startup_*.s`、`*.ld`、`cmake/` 整包覆盖过来，
   `Common/ Bsp/ App/ docs/ CMakeLists.txt` 保持不动
7. **改 `App/AppInit.cpp`**：
   - 头文件：`usart.h` / `i2c.h` / `can.h` / `tim.h` 按新工程改
   - 句柄：`&huart1`、`&hi2c1`、`&hcan1`（CubeMX 版本不同可能是 `hcan`）、`&htim3` 换成新工程的实际名字
   - LED 引脚：`BoardLed led(GPIOx, GPIO_PIN_y, active_low)`
8. **改 `CMakeLists.txt` 的芯片宏**：`-DSTM32F103xB` → 新芯片宏（CubeMX 生成的 `cmake/stm32cubemx/CMakeLists.txt` 里已有）

### 4.3 需要重点确认的地方

| 检查项 | 原因 |
|---|---|
| `configTOTAL_HEAP_SIZE` ≥ 10 KB | 默认 3072 字节放不下 3 个任务 + 队列；已在 `USER CODE BEGIN Defines` 覆盖 |
| 启动任务栈 ≥ 2 KB | 默认 128 words = 512 字节，`App_Init` 会栈溢出（本项目踩过） |
| 中断优先级 ≤ 5 的数值 | FreeRTOS 的 `configMAX_SYSCALL_INTERRUPT_PRIORITY = 5`，高于它的中断里不能调 `FromISR` API |
| `Stm32Timer` 只适用于 APB1 定时器 | 时钟源按 PCLK1 计算；若用 APB2 的 TIM1/TIM8 需改 `TimerClockHz()` |
| DMA 通道与外设对应关系 | 新芯片的 USART 可能挂在别的 DMA 请求上，CubeMX 会自动处理 |

### 4.4 移植后的验收清单

- [ ] 串口能打印 `framework boot` / `framework ready`
- [ ] 发 5 字节立即回显（DMA+IDLE 生效）
- [ ] PC13（或新板 LED）每秒翻转，日志出现 `[HB] tick=5`
- [ ] 拔掉串口线 / 波特率不匹配时，日志出现 `code=TIMEOUT`，不是卡死
- [ ] `cmake --build build` 0 warning / 0 error
