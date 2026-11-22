# 分层重构记录（Step 2）

> 本文档记录 2026-11-22 这次把 `Common/` 一锅端拆成 **Module / BSP / Application** 三层的改动。
> 对应任务书要求：① 按 BSP / Module / Application 分层，业务代码不直接调用 HAL。

---

## 1. 为什么改

改造前的结构：

```text
Common/
├── ErrorCode.hpp / Logger.*      # 接口 + 工具 + 日志
├── IUart / II2c / ICan           # 纯虚接口
├── Stm32Uart / Stm32I2c / Stm32Can  # 直接包含 usart.h / i2c.h / can.h，调 HAL
├── UartTask                      # 任务里直接调 HAL_GPIO_WritePin、vTaskDelay
└── AppInit.cpp
```

问题：

1. **层次是假的**——接口、HAL 实现、业务任务全在同一个目录里，评审一眼就能看出没分层；
2. **业务层直接调 HAL**——`UartTask::Run()` 里有 `HAL_GPIO_WritePin`，违反任务书第 1 条；
3. **换芯片无从下手**——`huart1`、`hi2c1` 这些 CubeMX 句柄散落在业务代码里。

改造后，三层的依赖方向是单向的：

```text
Application (App/)  ──依赖──▶  Module (Common/)  ◀──实现──  BSP (Bsp/)
       │                                                    │
       └────────────────── 组装（AppInit）───────────────────┘
```

- `Common/` 不认识任何 HAL 类型，只有接口、错误码、日志；
- `Bsp/` 才允许 `#include "usart.h"`、`tim.h`、`can.h`，才允许出现 `HAL_xxx`；
- `App/` 只拿接口引用（`IUart&`、`ICan&`、`ITimer&`），拿不到 HAL 句柄。

---

## 2. 目录变化

用 `git mv` 完成的迁移，历史记录连续（`git log --follow` 可以追到改造前）：

| 原路径 | 新路径 |
|---|---|
| `Common/Stm32Uart.hpp/.cpp` | `Bsp/Stm32Uart.hpp/.cpp` |
| `Common/Stm32I2c.hpp/.cpp` | `Bsp/Stm32I2c.hpp/.cpp` |
| `Common/Stm32Can.hpp/.cpp` | `Bsp/Stm32Can.hpp/.cpp` |
| `Common/UartTask.hpp/.cpp` | `App/UartEchoTask.hpp/.cpp` |
| `Common/AppInit.cpp` | `App/AppInit.cpp` |

最终结构：

```text
My_framework/
├── Common/                 # Module 层：只有接口与通用组件，零 HAL 依赖
│   ├── ErrorCode.hpp       #   统一错误码 + 转字符串
│   ├── IUart.hpp / II2c.hpp / ICan.hpp / ITimer.hpp
│   ├── Logger.hpp/.cpp     #   日志（走 IUart 接口，不依赖 printf）
│   └── ...                 #   （接口全部为纯虚基类）
├── Bsp/                    # BSP 层：唯一允许出现 HAL 的地方
│   ├── Stm32Uart.hpp/.cpp  #   UART：阻塞发送 + DMA&IDLE 接收
│   ├── Stm32I2c.hpp/.cpp   #   I2C：阻塞 + 中断
│   ├── Stm32Can.hpp/.cpp   #   CAN：阻塞发送 + 中断接收
│   ├── Stm32Timer.hpp/.cpp #   TIM：周期中断
│   └── BoardLed.hpp/.cpp   #   板载 LED，业务层用它代替 HAL_GPIO_xxx
├── App/                    # Application 层：只依赖接口
│   ├── AppInit.cpp         #   组合根：唯一出现 huart1/hi2c1/hcan1/htim3 的文件
│   ├── UartEchoTask.*      #   串口回显任务（队列）
│   ├── CanEchoTask.*       #   CAN 回显任务（队列）
│   └── HeartbeatTask.*     #   心跳任务（二值信号量，1s）
├── Core/ Drivers/ Middlewares/   # CubeMX 生成，保持原样
└── CMakeLists.txt
```

---

## 3. 每个文件干了什么

### Module 层（Common/）

| 文件 | 职责 | 关键点 |
|---|---|---|
| `ErrorCode.hpp` | 11 个错误码 + `ErrorCodeToString` | `enum class : uint32_t`，零隐式转换 |
| `IUart.hpp` | `Init` / `Transmit`（阻塞）/ `StartReceive`（DMA+IDLE） | 回调类型 `void(*)(void*, const uint8_t*, uint16_t)`，明确"ISR 上下文" |
| `II2c.hpp` | 阻塞读写 + 异步读写 + `WaitAsyncDone` | 每条路径都带 `timeout_ms` 或返回码 |
| `ICan.hpp` | `CanFrame` 结构 + `Init/Transmit/StartReceive` | `dlc` 上限 8，`extended` 区分标准/扩展帧 |
| `ITimer.hpp` | 周期定时器，回调在中断上下文 | 只做"给信号量"这类短动作 |
| `Logger.*` | 格式化打印到 `IUart`，带互斥量 | 不再走 `printf`，彻底避开 `__io_putchar` 空符号 |

### BSP 层（Bsp/）

| 文件 | 实现要点 |
|---|---|
| `Stm32Uart` | `HAL_UART_Transmit`（阻塞）+ `HAL_UART_Receive_DMA` 配 `UART_IT_IDLE`；空闲中断里用 `CNDTR` 算出这一帧的**实际长度**，收完再重新武装；`HAL_UART_ErrorCallback` 里清 ORE 并重启 DMA（否则溢出后接收直接停摆） |
| `Stm32I2c` | 阻塞：`HAL_I2C_Mem_Write/Read`；中断：`HAL_I2C_Mem_Write_IT` + 二值信号量；`Init` 是**补上的**（旧代码只有声明没有定义，一调用就链接失败） |
| `Stm32Can` | 32 位掩码全 0 的滤波器（收所有帧）+ `HAL_CAN_Start` + FIFO0 中断；发送前检查邮箱空闲，超时返回 `ERR_TIMEOUT`；修掉了旧文件里 `can-HandleTypeDef`、`CAN_TxHeaderTYpeDef`、`IF`、`ERR_TIMROUT:` 等一堆编译不过的笔误 |
| `Stm32Timer` | 按 `PCLK1` 自动算预分频（APB1 分频 ≠1 时定时器时钟 ×2），计数器固定 10kHz，`ARR = period_ms × 10 - 1`，所以换主频不用改代码 |
| `BoardLed` | 把 `HAL_GPIO_WritePin/TogglePin` 收进 BSP，业务层只调 `On/Off/Toggle` |

### Application 层（App/）

| 文件 | 优先级 | 栈 | 通信方式 |
|---|---|---|---|
| `UartEchoTask` | 3 | 384 words | 队列 8×66B（ISR → 任务） |
| `CanEchoTask` | 2 | 320 words | 队列 8×`CanFrame` |
| `HeartbeatTask` | 1 | 256 words | 二值信号量（TIM 中断 → 任务） |

`AppInit.cpp` 是**组合根**：所有对象在这里用 `static` 局部变量构造，初始化失败一律 `Logger::Error` 后返回错误码，`freertos.c` 里的默认任务调完就 `vTaskDelete(NULL)` 自删，省回 512 字节栈。

---

## 4. 两个"渐进式"设计

### 4.1 CAN / TIM 还没在 CubeMX 里配也能编过

`CMakeLists.txt` 里用文件存在性做开关：

```cmake
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/can.h")
    list(APPEND FRAMEWORK_SOURCES Bsp/Stm32Can.cpp)
endif()
```

`AppInit.cpp` 里用 `__has_include` 对应地跳过调用：

```cpp
#if __has_include("can.h")
#  include "can.h"
#  define APP_HAS_CAN 1
#endif
```

所以现在的状态是：**串口 + I2C 已经能跑，CAN/TIM 等你配完 CubeMX 重新 `cmake` 一次就自动接上**，不会出现"配了一半编译不过"的僵局。

### 4.2 堆从 3072 改成 10240

`configTOTAL_HEAP_SIZE` 原来是 3072 字节，而 idle 任务 512B + 软件定时器任务 1024B + UartTask 2048B + 队列 680B 已经超了——`xTaskCreate` 一直在**静默失败**（返回值没检查）。现在 10240B，默认任务启动完还会自删。

> ⚠️ 这个宏在 CubeMX 生成区里。重新生成后如果变回 3072，去 `FreeRTOS → Config parameters → TOTAL_HEAP_SIZE` 改成 10240，这样 `.ioc` 里也会记下来。

---

## 5. 编译验证

```text
Memory region         Used Size  Region Size  %age Used
             RAM:       16040 B        20 KB     78.32%
           FLASH:       36540 B        64 KB     55.76%
```

`cmake --build build --clean-first` 全量重编，**0 warning / 0 error**（开了 `-Wall`）。C++ 侧加了 `-fno-exceptions -fno-rtti -fno-threadsafe-statics`。

---

## 6. 还没做的（下一步）

| # | 事项 | 说明 |
|---|---|---|
| 1 | CubeMX 配 CAN + TIM3 + I2C 中断 | 见下节清单；配完 `cmake -S . -B build` 会自动启用两个驱动 |
| 2 | CAN 回环模式 | CubeMX 6.18 的 F1 CAN 面板没有 Operating Mode，生成后手动把 `can.c` 里 `hcan1.Init.Mode` 改成 `CAN_MODE_LOOPBACK`（只改这一行，其余别动） |
| 3 | `docs/Architecture.md` | 目录树已同步更新，层次说明可以再补一版 |
| 4 | 测试视频 | 串口回显 / 心跳灯 / CAN 回环 / 超时日志 |
| 5 | git push | 本机领先 origin 若干 commit，记得推 |

### CubeMX 待办清单

- `Connectivity → CAN1`：Activated、PA11/PA12、500 kbit/s、NVIC 勾 **CAN RX0 interrupt**（优先级 5）
- `Timers → TIM3`：Internal Clock、NVIC 勾 **TIM3 global interrupt**（优先级 5）
- `Connectivity → I2C1`：NVIC 勾 **I2C1 event + error interrupt**（优先级 5）
- `FreeRTOS → Config parameters`：**TOTAL_HEAP_SIZE = 10240**
- `GPIO → PA11`：Pull-up（回环自测时防悬空噪声）

---

## 7. 回滚

```powershell
git reset --hard HEAD~1      # 回到改造前
# 或者只丢弃工作区改动
git checkout -- .
```

改造前的最后一次提交是 `8726477 feat: implement I2C driver ...`。
