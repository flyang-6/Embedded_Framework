# STM32CubeMX 配置详解（本项目逐项讲解）

> 面向：想在 CubeMX 里"知道自己每个勾选在干什么"的阶段。
> 对照工程：`My_framework.ioc`，芯片 STM32F103C8T6。

---

## 0. 先搞清三件事

| 东西 | 是什么 | 注意 |
|---|---|---|
| `.ioc` 文件 | CubeMX 的工程配置，本质是个键值对文本 | **要提交进 git**，否则别人重建不了外设 |
| 生成出来的代码 | `Core/`、`Drivers/`、`Startup`、`cmake/` | 大部分会被反复覆盖 |
| `USER CODE BEGIN/END` 区 | 生成器**不会动**的注释块 | 你所有手写代码都必须放这里面，否则下次生成就没了 |

生成器只保护 `USER CODE` 区。判断方法：看到 `/* USER CODE BEGIN ... */` 和 `/* USER CODE END ... */` 之间就是安全区。

---

## 1. RCC（时钟树）—— 所有外设速度的源头

路径：`Pinout & Configuration → System Core → RCC`，以及上方 **Clock Configuration** 页。

| 参数 | 本项目取值 | 含义 |
|---|---|---|
| High Speed Clock (HSE) | Disable | 外部晶振。Blue Pill 板上有 8MHz 晶振，但本工程没用 |
| Low Speed Clock (LSE) | Disable | 32.768kHz，给 RTC 用 |
| HSI | 8MHz 内部 RC | 上电默认时钟源，精度 ±1% |
| PLL | 关闭 | 倍频器。打开后可把 8MHz 倍频到 72MHz |
| SYSCLK | 8MHz（HSI） | CPU 主频 |
| AHB 分频 | /1 → HCLK 8MHz | 总线时钟 |
| APB1 分频 | /1 → PCLK1 8MHz | 低速外设（USART2/3、I2C、CAN、TIM2~TIM4） |
| APB2 分频 | /1 → PCLK2 8MHz | 高速外设（USART1、SPI1、TIM1、GPIO） |
| FLASH Latency | 0 WS | 8MHz 不需要等待周期；72MHz 要 2WS |

**两个必须记住的规则：**

1. **串口波特率、I2C 速率、CAN 位时序，全都从 PCLK 推出来**。改主频必须重新核对这三个。
2. **APB 分频不为 1 时，定时器时钟 = PCLK × 2**。比如 72MHz 系统里 APB1 = 36MHz，
   TIM2~TIM4 实际按 72MHz 计数。代码里 `TimerClockHz()` 就是干这个的：

```cpp
const uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> 8;
return (ppre1 >= 4U) ? (pclk1 * 2U) : pclk1;   // 分频了就乘 2
```

为什么本项目只用 8MHz？因为最小验证不需要性能，8MHz 下串口/I2C/CAN 都能算得很整齐，
而且 HSI 不用外部晶振，少一个硬件依赖。**接真实 CAN 总线时建议改 HSE+PLL 到 72MHz**，HSI 的 ±1% 精度做 CAN 会有偶发错误帧。

---

## 2. SYS —— 调试口和时基

路径：`System Core → SYS`

| 参数 | 取值 | 含义 |
|---|---|---|
| Debug | **Serial Wire** | 只保留 SWD 两线（PA13=SWDIO、PA14=SWCLK），释放 PA15/PB3/PB4 |
| Timebase Source | **SysTick** | HAL 的 `HAL_GetTick()` 和超时判断都靠它 |

**为什么 Timebase 会变成一个选项？** 因为 FreeRTOS 也要用 SysTick 做任务调度。
两者抢同一个中断会冲突。本项目用的是 `SysTick` + FreeRTOS 的 `USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION=1`
（在 `FreeRTOSConfig.h` 里），由 `stm32f1xx_it.c` 的 `SysTick_Handler` 同时调 `HAL_IncTick()` 和 `xPortSysTickHandler()`。

另一个常见做法是 Timebase 选 `TIM6`，把 SysTick 完全让给 FreeRTOS。

---

## 3. GPIO —— 引脚模式

路径：`System Core → GPIO`，或直接在芯片图上点引脚。

| 引脚 | 配置 | 为什么 |
|---|---|---|
| PC13 | Output Push Pull，Low speed | 板载 LED（低电平点亮，所以 `BoardLed` 里 `active_low=true`） |
| PA9 | Alternate Function Push Pull | USART1_TX |
| PA10 | Input，No pull | USART1_RX |
| PB6 / PB7 | Alternate Function **Open Drain** | I2C1_SCL / SDA，I2C 必须开漏 |
| PA11 | Input，No pull（建议改 Pull-up） | CAN_RX，回环自测时悬空容易收噪声 |
| PA12 | Alternate Function Push Pull | CAN_TX |
| PA13 / PA14 | （SYS 里自动配） | SWD 调试口 |
| PA0 / PA1 | **本项目没在 CubeMX 配** | 由 `Bsp/Stm32Pwm` 运行时配成 AF_PP 给 TIM2_CH1/CH2 |

**模式含义速查：**

| 模式 | 什么时候用 |
|---|---|
| `Input` | 读电平：按键、CAN_RX、外部中断 |
| `Output Push Pull` | 推挽输出：LED、普通控制脚 |
| `Output Open Drain` | 需要线与/上拉时：I2C 模拟、共享总线 |
| `Alternate Function Push Pull` | 外设自己驱动引脚：USART_TX、SPI、TIM_PWM |
| `Alternate Function Open Drain` | 开漏外设：I2C |
| `Analog` | ADC 输入 |

`Pull-up / Pull-down`：内部上拉/下拉，省外部电阻；`No pull` 就是都不接。
`Speed`：引脚翻转速度，只有高速信号（SPI、CAN、PWM）才需要 High。

---

## 4. USART1 —— 串口 + DMA

路径：`Connectivity → USART1`

### 4.1 参数（Parameter Settings）

| 参数 | 取值 | 含义 |
|---|---|---|
| Mode | **Asynchronous** | 异步串口（不需要时钟线）。其它选项是同步/半双工/多机 |
| Baud Rate | 115200 | 波特率。8MHz 下分频误差约 0.04%，安全 |
| Word Length | 8 Bits | 数据位 |
| Parity | None | 校验位。串口助手一般是 8N1 |
| Stop Bits | 1 | 停止位 |
| Data Direction | Receive and Transmit | 收发都要 |
| Over Sampling | 16 | 每位采 16 个点取中间，抗噪更好；8 倍可以跑更高波特率 |

### 4.2 DMA（DMA Settings 页）

| 参数 | 取值 | 含义 |
|---|---|---|
| DMA Request | USART1_RX | 只给接收配了 DMA（F103 的 USART1_TX 也能配，本工程发送用阻塞） |
| Channel | DMA1_Channel5 | 固定映射，CubeMX 自动选 |
| Direction | Peripheral To Memory | 外设 → 内存 |
| Mode | **Normal** | 传完 N 个字节就停（不是 Circular 循环）。停了我们好判断"这一包收完" |
| Data Width | Byte / Byte | 逐字节搬 |
| Priority | Low | DMA 通道优先级，不是中断优先级 |

`Circular` 模式永远不会停，适合"环形缓冲 + 定时取快照"；
本工程用 `Normal` + **空闲中断**判断一帧结束，短帧也不会卡住。

### 4.3 NVIC（NVIC Settings 页）

勾选 **USART1 global interrupt**，Preemption Priority = **5**。

> **5 这个数字有硬性约束**：FreeRTOS 的 `configMAX_SYSCALL_INTERRUPT_PRIORITY = 5`，
> 凡是中断里要调用 `xQueueSendFromISR` 这类 API 的中断，优先级数值必须 **≥ 5**（数值越小优先级越高）。
> 设成 0~4 会在运行时被 `configASSERT` 抓住直接死循环。

### 4.4 空闲中断（IDLE）为什么在 CubeMX 里找不到

CubeMX 只提供 RXNE / TXE / 错误中断的勾选项，**IDLE 中断没有 UI**，只能在代码里开：

```cpp
__HAL_UART_ENABLE_IT(huart_, UART_IT_IDLE);
```

这就是 `Stm32Uart::StartReceive()` 里那行的由来；同时要在 `USART1_IRQHandler` 里
先判 IDLE 再交给 `HAL_UART_IRQHandler`。

---

## 5. I2C1 —— 100kHz 标准模式

路径：`Connectivity → I2C1`

| 参数 | 取值 | 含义 |
|---|---|---|
| I2C Speed Mode | Standard Mode | 标准模式（100k）；Fast Mode 是 400k |
| Clock Speed | 100000 Hz | SCL 频率 |
| Duty Cycle | 2 | 只在 Fast Mode 下有意义（Tlow/Thigh 比例） |
| Addressing Mode | 7-bit | 7 位地址；10 位很少用 |
| Dual Address Mode | Disable | 双地址（从机才需要） |
| General Call | Disable | 广播地址 0x00 |
| No Stretch | Disable | 允许从机拉低 SCL 拖时钟（正常 I2C 行为） |

**NVIC**：勾 **I2C1 event interrupt** 和 **I2C1 error interrupt**，优先级 5。
只做阻塞读写其实不用开中断；本工程开了，是为了支持 `WriteRegAsync/ReadRegAsync` 的中断方式。

> 注意：代码里 `dev_addr << 1` 是 STM32 HAL 的要求——HAL 要 8 位地址（含读写位），
> 而 `II2c` 接口对外暴露的是 7 位地址。

---

## 6. CAN1 —— 位时序 + 滤波器 + 模式

路径：`Connectivity → CAN1`

### 6.1 位时序（Bit Timings）

CAN 把 1 个位切成若干 **时间片 tq**，一个位 = `1(SYNC) + BS1 + BS2` 个 tq。

```
波特率 = PCLK1 / (Prescaler × (1 + BS1 + BS2))
```

本项目（PCLK1 = 8MHz）：

```
hcan.Init.Prescaler = 1;
hcan.Init.TimeSeg1  = CAN_BS1_13TQ;   // BS1
hcan.Init.TimeSeg2  = CAN_BS2_2TQ;    // BS2
→ 8MHz / (1 × (1+13+2)) = 500 kbit/s
```

**采样点** = `(1 + BS1) / 总 tq` = 14/16 = **87.5%**（CAN 推荐 75%~87.5%）。

> CubeMX 默认生成的是 `Prescaler=16 / BS1=1TQ / BS2=1TQ`，算下来约 166kbit/s，且采样点只有 66%，
> 跟任何标准设备都对不上。这是本项目踩过的坑之一。

### 6.2 工作模式（Operating Mode）

| 模式 | 行为 | 用途 |
|---|---|---|
| Normal | 正常收发，必须接收发器 + 另一节点 | 真实总线 |
| Loopback | 发出的帧内部回环到自己 | **无收发器自测**（本项目） |
| Silent | 只听不发 | 监听总线、调试 |
| Silent Loopback | 只听 + 回环 | 自测且不影响总线 |

CubeMX 6.18 的 F1 CAN 面板**没有这个选项**，只能在 `can.c` 里手改：

```c
hcan.Init.Mode = CAN_MODE_LOOPBACK;   // 接真实收发器时改回 CAN_MODE_NORMAL
```

### 6.3 其它参数

| 参数 | 本项目 | 含义 |
|---|---|---|
| Time Triggered Mode | Disable | 时间触发 CAN（TTCAN），基本不用 |
| Auto Bus Off | Disable | 总线关闭后自动恢复。量产建议 Enable |
| Auto Wake Up | Disable | 总线活动自动唤醒（低功耗） |
| Auto Retransmission | Disable | 发送失败自动重发。**注意**：回环模式下必须 Disable，否则会一直重发 |
| Receive Fifo Locked | Disable | FIFO 满时覆盖旧帧（Enable 则丢弃新帧） |
| Transmit Fifo Priority | Disable | 按请求顺序发送；Enable 则 ID 小的优先 |

### 6.4 滤波器（代码里配，不在 CubeMX）

```c
filter.FilterMode = CAN_FILTERMODE_IDMASK;
filter.FilterScale = CAN_FILTERSCALE_32BIT;
filter.FilterIdHigh/Low = 0; filter.FilterMaskIdHigh/Low = 0;   // 掩码全 0 = 全通
filter.FilterBank = 0;
filter.FilterFIFOAssignment = CAN_RX_FIFO0;
filter.FilterActivation = ENABLE;
```

F103 有 14 组滤波器（bank 0~13），每组可以配成"ID + 掩码"的形式。掩码位为 1 表示"这一位必须匹配"。

### 6.5 中断

NVIC 里勾 **CAN RX0 interrupt**，优先级 5。F1 上它和 USB 共用一个向量，所以中断服务函数叫
`USB_LP_CAN1_RX0_IRQHandler`，这是正常的。

---

## 7. TIM —— 定时器怎么算

### 7.1 三个公式

```
计数频率 = 定时器时钟 / (PSC + 1)
溢出周期 = (ARR + 1) / 计数频率
占空比   = CCR / (ARR + 1)
```

### 7.2 TIM3（心跳，1s）

本项目由 `Stm32Timer` 在运行时设置（CubeMX 里的 PSC/ARR 会被覆盖）：

```cpp
psc  = 8MHz / 10000 = 800     → PSC = 799     // 计数器 10kHz（0.1ms 一格）
ARR  = 1000ms × 10 - 1 = 9999                // 1s
```

CubeMX 里只需要：`Timers → TIM3 → Clock Source = Internal Clock` + NVIC 勾 **TIM3 global interrupt**（优先级 5）。
PSC/ARR 填什么都不影响，因为代码会重算。

### 7.3 TIM2（舵机 PWM，50Hz）

本项目**故意不经过 CubeMX**，由 `Bsp/Stm32Pwm::Init()` 自己完成：

```cpp
psc  = 8MHz / 1MHz = 8        → PSC = 7       // 计数器 1MHz，1 个数 = 1us
ARR  = 1MHz / 50Hz - 1 = 19999                // 20ms 周期
CCR  = 500..2500                              // 0.5ms~2.5ms 脉宽 → 0°~180°
```

好处：`PSC` 按主频自动算，换到 72MHz 的板子也不用改代码；也证明了 BSP 层不绑死 CubeMX 生成物。

### 7.4 PWM 相关选项

| 选项 | 含义 |
|---|---|
| PWM Generation CHx | 该通道输出 PWM（对比 Complementary 是互补输出，驱动半桥用） |
| PWM Mode 1 / 2 | Mode1：计数值 < CCR 时输出有效电平；Mode2 反过来 |
| Pulse | 就是 CCR 初值 |
| Output compare preload | CCR 双缓冲，改占空比时不会出现半个周期的毛刺 |
| Fast Mode | 关掉，一般不用 |

---

## 8. FreeRTOS —— 任务、堆、优先级

路径：`Middleware → FREERTOS`，Interface 选 **CMSIS_V2**。

### 8.1 Tasks 列表（Tasks and Queues 页）

| 字段 | 含义 |
|---|---|
| Task Name | 任务名，`osThreadNew` 里用 |
| Priority | `osPriorityNormal` = 24（CMSIS 枚举），不是 FreeRTOS 原始数值 |
| Stack Size | **单位是 words（4 字节）**。`128` = 512 字节 |
| Entry Function | 任务函数 |
| Allocation | Dynamic（用 heap） |

> 本项目踩过的坑：默认 `128`（512 字节）的启动任务里跑 `App_Init()`，栈溢出把堆踩坏，
> 表现为"打印完启动日志就死机"。现已改成 `512`（2KB）。

### 8.2 Config parameters 页

| 参数 | 本项目 | 含义 |
|---|---|---|
| TOTAL_HEAP_SIZE | 3072（生成值）→ 代码里覆盖成 10240 | FreeRTOS 堆，任务栈 + 队列都从这里分 |
| MAX_PRIORITIES | 56 | 优先级档数 |
| MINIMAL_STACK_SIZE | 128 words | 空闲任务栈 |
| TICK_RATE | 1000 Hz | 1 tick = 1ms，`pdMS_TO_TICKS(1000)` = 1000 |
| USE_TIMERS | Enabled | 软件定时器服务任务（占 1KB 堆） |
| USE_MUTEXES / COUNTING_SEMAPHORES | Enabled | 互斥量、计数信号量 |
| MAX_SYSCALL_INTERRUPT_PRIORITY | 5 | 能调 `FromISR` API 的最高中断优先级（数值最小） |

**优先级数值规则**：FreeRTOS 里**数字越大优先级越高**；NVIC 里**数字越小优先级越高**。两套规则相反，最容易搞混。

本工程实际分配：

| 任务 | FreeRTOS 优先级 | 栈 | 理由 |
|---|---|---|---|
| UartEchoTask | 3 | 384 w | 串口交互时延最敏感 |
| CanEchoTask | 2 | 320 w | CAN 允许毫秒级抖动 |
| ServoTask | 1 | 320 w | 舵机机械响应 ~100ms |
| HeartbeatTask | 1 | 256 w | 纯指示 |

### 8.3 为什么堆要改到 10KB

idle(512B) + 定时器任务(1KB) + 四个任务栈(≈5KB) + 队列/信号量(≈1.5KB) + TCB，
3072 字节根本不够，`xTaskCreate` 会静默返回失败。CubeMX 重新生成会把 `/ioc` 里的值写回
`FreeRTOSConfig.h`，所以本项目把覆盖写进了 `USER CODE BEGIN Defines` 区：

```c
#undef  configTOTAL_HEAP_SIZE
#define configTOTAL_HEAP_SIZE   ((size_t)10240)
```

---

## 9. NVIC 优先级总表（本项目）

| 中断 | 优先级 | 为什么是这个值 |
|---|---|---|
| USART1 | 5 | 要在 ISR 里 `xQueueSendFromISR`，必须 ≥ 5 |
| DMA1_Channel5 | 5 | 同上（DMA 完成 → 串口回调） |
| USB_LP_CAN1_RX0 | 5 | 同上 |
| TIM3 | 5 | 同上（给信号量） |
| I2C1 event / error | 5 | 同上 |

规则一句话：**凡是要和 FreeRTOS 打交道的 ISR，数值都设 5；不需要的可以设 0~4 换更低延迟。**

---

## 10. Project Manager —— 生成什么

| 选项 | 本项目 | 说明 |
|---|---|---|
| Toolchain / IDE | **CMake** | 生成 `CMakeLists.txt` + `cmake/stm32cubemx` |
| Application Structure | Advanced（默认） | 是否拆分 `Core/Inc` 与 `Core/Src` |
| Generate Under Root | 是 | 直接生成到工程根 |
| Copy only necessary library files | 是 | 只拷用到的 HAL 源文件（省体积） |

**每次点 GENERATE 会发生什么：**

- `Core/Src/*.c` 重新生成，`USER CODE` 区内容保留
- `cmake/stm32cubemx/CMakeLists.txt` 重写，新外设源文件自动加进来
- `FreeRTOSConfig.h` 重新生成（`TOTAL_HEAP_SIZE` 会按 `.ioc` 还原）
- **`can.c` 里手改的 Mode/位时序会被还原**（本项目已知，写在文档里）

---

## 11. 哪些会被覆盖（速查表）

| 文件/位置 | 重新生成后 |
|---|---|
| `USER CODE BEGIN/END` 之间 | ✅ 保留 |
| `main.c` 的 `MX_xxx_Init()` 调用顺序 | ❌ 重写 |
| `can.c` 的 `Init.Mode` / 位时序 | ❌ 重写 |
| `FreeRTOSConfig.h` 的 `TOTAL_HEAP_SIZE` | ❌ 重写（除非写在 USER CODE 区） |
| `cmake/stm32cubemx/CMakeLists.txt` | ❌ 重写（自动带上新外设） |
| 自己写的 `Common/ Bsp/ App/` | ✅ 完全不受影响 |
| `CMakeLists.txt`（根目录） | ✅ 不生成，保留 |

---

## 12. FAQ

**Q1：为什么我的程序只跑 8MHz？会不会太慢？**
够用。这个工程没有算力需求，外设速率都由分频算出来，8MHz 下串口/I2C/CAN 都很整。要跑 DSP 或高速采样再上 72MHz。

**Q2：CubeMX 里改了 PSC，代码里又算一遍，到底听谁的？**
听代码的。CubeMX 只是生成初值，`Stm32Timer::Start()` / `Stm32Pwm::Init()` 会按当前 PCLK 重算，所以换主频不用改 CubeMX。

**Q3：为什么中断优先级都设 5？设 0 不行吗？**
设 0 比 `configMAX_SYSCALL_INTERRUPT_PRIORITY` 还高，在 ISR 里调 FreeRTOS API 会触发断言死循环。

**Q4：DMA 用 Normal 还是 Circular？**
要"一帧一帧处理"就用 Normal + 空闲中断（本项目）；要持续采样（ADC）用 Circular。

**Q5：CAN 为什么必须 Loopback 才能自测？**
因为正常模式需要总线上有另一个节点回 ACK，没有收发器/第二个节点就发送失败。Loopback 时控制器自己给自己回 ACK。

**Q6：为什么 TIM2 没在 CubeMX 里配？**
为了让 BSP 层自己持有时钟/GPIO/时基初始化，换芯片时少一步 CubeMX 操作，也证明框架不绑死生成代码。

**Q7：`.ioc` 要不要提交？**
必须提交。它是"外设配置的唯一真相"，没有它别人 clone 下来只有代码、没有外设定义。

**Q8：改了 `.ioc` 但生成后代码没变？**
CubeMX 有时不会覆盖已存在的文件：看 `Project Manager → Code Generator → Keep User Code when re-generating`，以及是否只点了"Generate Code"没点"Save"。
