# 工程文件地图：每个文件到底是干什么的

> 读法建议：先看第 0 节（`.h` / `.cpp` / `.s` / `.ld` 是什么），再看第 5 节（自己写的代码），
> 最后按需查第 2~4 节的生成文件。

---

## 0. 先弄清文件类型

| 后缀 | 是什么 | 编译后变成 |
|---|---|---|
| `.h` / `.hpp` | **头文件**：只放"声明"（函数原型、类定义、宏、extern 变量），被 `#include` 展开到引用它的源文件里 | 不单独编译，不产生代码 |
| `.c` / `.cpp` | **源文件**：放"实现"（函数体），每个 `.c`/`.cpp` 单独编译成一个 `.o` | `.o` 目标文件 |
| `.s` | **汇编源文件**：启动文件，含中断向量表和复位入口 | `.o` |
| `.ld` | **链接脚本**：告诉链接器 Flash 从 `0x08000000` 开始、RAM 从 `0x20000000` 开始，各段怎么摆 | 不编译，链接阶段用 |
| `.ioc` | CubeMX 工程配置（文本） | 不参与编译，但决定了生成什么代码 |
| `.json` / `.txt` | 构建配置（CMake / preset） | 不参与编译 |

**几个必知的关键字：**

```cpp
#pragma once          // 头文件只展开一次（防重复包含），等价于 include guard
inline                // 允许多个 .o 里出现同一函数定义（header-only 用它）
static（文件内）        // 这个函数/变量只在本 .cpp 可见，别的文件链接不到
extern "C"            // 按 C 的规则生成符号名，供 C 代码调用（C++ 会做名字修饰）
extern                // 声明"变量/函数在别处定义"
```

`.h` 里**不要**写会分配内存的定义（比如 `int arr[100];`、非 inline 的函数体），否则多个 `.cpp` 包含它就会重复定义报错。

---

## 1. 三圈划分

```
My_framework/
├── Common/ Bsp/ App/     ← ① 你写的代码（唯一需要天天改的）
├── Core/                 ← ② CubeMX 生成（外设初始化 + 中断 + 启动）
├── Drivers/              ← ③ ST 官方 HAL/CMSIS（基本不动）
├── Middlewares/FreeRTOS/ ← ③ FreeRTOS 源码（基本不动）
├── startup_*.s / *.ld    ← ② 启动文件 / 链接脚本
├── cmake/                ← ② 构建脚本（一部分生成）
└── docs/                 ← 文档
```

---

## 2. 根目录文件

| 文件 | 作用 | 你会改它吗 |
|---|---|---|
| `My_framework.ioc` | CubeMX 工程：引脚、外设、时钟、FreeRTOS 配置的**唯一真相** | 配外设时由 CubeMX 改，要提交 |
| `CMakeLists.txt` | 顶层构建脚本：C++17、三个 include 目录、框架源文件清单、编译选项 | 加/删源文件时改 |
| `CMakePresets.json` | VS Code CMake 插件用的预设（Debug/Release、Ninja、工具链路径） | 基本不动 |
| `startup_stm32f103xb.s` | 启动汇编：中断向量表、复位后调 `SystemInit` → `__libc_init_array` → `main` | 不动 |
| `STM32F103xx_FLASH.ld` | 链接脚本：64KB Flash / 20KB RAM 的地址与段布局，栈顶地址 | 换芯片要换 |
| `.mxproject` | CubeMX 的内部缓存（上次生成的文件列表） | 不动，CubeMX 自己维护 |
| `.gitignore` | 忽略 `build/`、`.vscode` 等 | 按需 |
| `test_main.cpp` | ⚠️ 早期 PC 端跑 Logger 的遗留文件，**没进 CMake、且接口已过时（编译不过）** | 建议删掉或改成真正的 host 测试 |

---

## 3. `Core/Inc/`（头文件）

| 文件 | 里面有什么 |
|---|---|
| `main.h` | `Error_Handler()` 声明、全局宏、包含 `stm32f1xx_hal.h` |
| `stm32f1xx_hal_conf.h` | **HAL 模块总开关**（`HAL_CAN_MODULE_ENABLED` 等）+ HSE 频率 + 断言配置。编译哪些 `hal_xxx.c` 由它决定 |
| `stm32f1xx_it.h` | 中断服务函数的声明（主要给 C 文件之间互相引用） |
| `FreeRTOSConfig.h` | FreeRTOS 全部配置：堆大小、优先级档数、tick 频率、SysTick 接管方式 |
| `usart.h` | `extern UART_HandleTypeDef huart1;` + `MX_USART1_UART_Init()` |
| `i2c.h` | `extern I2C_HandleTypeDef hi2c1;` + `MX_I2C1_Init()` |
| `can.h` | `extern CAN_HandleTypeDef hcan;`（注意：F1 单 CAN 叫 `hcan`，不是 `hcan1`） |
| `tim.h` | `extern TIM_HandleTypeDef htim3;` + `MX_TIM3_Init()` |
| `gpio.h` / `dma.h` | `MX_GPIO_Init()` / `MX_DMA_Init()` 声明 |

**这些头里的 `extern XXX_HandleTypeDef` 就是"外设句柄"**：HAL 的每个函数都要传它，句柄里存着寄存器基地址、状态、DMA 绑定等。

---

## 4. `Core/Src/`（源文件）

| 文件 | 作用 | 和你的代码的关系 |
|---|---|---|
| `main.c` | 上电入口：`HAL_Init` → `SystemClock_Config` → `MX_xxx_Init()` → `osKernelStart()` | 只在 `USER CODE` 区加东西 |
| `freertos.c` | CubeMX 版 FreeRTOS 初始化：建默认任务、`MX_FREERTOS_Init` | 默认任务里调 `App_Init()`，调完自删 |
| `stm32f1xx_it.c` | **所有中断服务函数**：`SysTick_Handler`、`USART1_IRQHandler`、`TIM3_IRQHandler`、`USB_LP_CAN1_RX0_IRQHandler`… | USART1 里加了 IDLE 钩子 |
| `stm32f1xx_hal_msp.c` | **MSP = MCU Support Package**：每个外设的"底层初始化"——开时钟、配 GPIO、绑 DMA、开 NVIC。HAL 在 `HAL_xxx_Init` 里回调它 | 不动（CubeMX 生成） |
| `usart.c` / `i2c.c` / `can.c` / `tim.c` | 各外设的 `MX_xxx_Init()` + `HAL_xxx_MspInit` | `can.c` 里 Mode/位时序被手改过 |
| `gpio.c` | 引脚初始化（PC13 输出等） | 不动 |
| `dma.c` | DMA 控制器总初始化 | 不动 |
| `system_stm32f1xx.c` | `SystemInit()`（复位后配时钟）+ `SystemCoreClock` 变量 | 不动 |
| `syscalls.c` | 给 newlib 补的**系统调用桩**：`_write`/`_read`/`_sbrk`/`_close`… 没它 `printf` 链接不过 | 不动（我们不用 printf 了） |
| `sysmem.c` | `_sbrk` 实现：把 `.bss` 之后的空闲 RAM 当堆分给 `malloc` | 不动 |

**`stm32f1xx_hal_msp.c` 和 `usart.c` 里的 `HAL_UART_MspInit` 是两处不同的东西**：CubeMX 把 MSP 回调写在各自外设的 `.c` 里（`usart.c` 的 `HAL_UART_MspInit`），`stm32f1xx_hal_msp.c` 里是 `HAL_MspInit`（全局的，配 AFIO 重映射等）。

---

## 5. 自己写的代码

### 5.1 `Common/` —— Module 层（零 HAL 依赖）

| 文件 | 回答什么问题 | 改动场景 |
|---|---|---|
| `ErrorCode.hpp` | "失败怎么表示？" 11 个错误码 + 转字符串 | 加新错误类型 |
| `IUart.hpp` | "串口应该提供哪些能力？" 纯虚接口 | 加新方法（如 `GetRxCount`） |
| `II2c.hpp` | "I2C 应该提供哪些能力？" 阻塞 + 异步 | 同上 |
| `ICan.hpp` | "CAN 帧长什么样？" `CanFrame` + 接口 | 加远程帧、时间戳 |
| `ITimer.hpp` | "周期定时器怎么抽象？" | — |
| `IPwm.hpp` | "PWM 怎么抽象？" 频率 + 占空比 + 脉宽 | — |
| `Logger.hpp/.cpp` | "日志往哪打、怎么格式化？" 通过 `IUart*` 输出 | 加级别、加时间戳 |
| `FrameCodec.hpp` | "一帧数据怎么界定、怎么校验？" `AA 55 LEN DATA CRC8` | 换协议 |
| `Servo.hpp/.cpp` | "舵机的角度怎么变成脉宽？" 角度→500~2500us | 换舵机量程 |

> `Common` 里出现 `#include "usart.h"` 就已经破功了——这是评审最容易查的点。

### 5.2 `Bsp/` —— 板级支持层（唯一碰 HAL 的地方）

| 文件 | 里面在干什么 |
|---|---|
| `Stm32Uart.hpp/.cpp` | `HAL_UART_Transmit`（阻塞）+ `HAL_UART_Receive_DMA` + IDLE 中断算帧长 + ORE 错误恢复 |
| `Stm32I2c.hpp/.cpp` | `HAL_I2C_Mem_Read/Write`（阻塞）+ `_IT` 版本 + 二值信号量等完成 |
| `Stm32Can.hpp/.cpp` | 滤波器全通 + `HAL_CAN_Start` + FIFO0 中断回调 → `CanFrame` |
| `Stm32Timer.hpp/.cpp` | 按 PCLK 算 PSC/ARR，`HAL_TIM_Base_Start_IT` + 回调分发 |
| `Stm32Pwm.hpp/.cpp` | 自己开时钟、配 GPIO 复用、`HAL_TIM_PWM_Init/ConfigChannel/Start` |
| `BoardLed.hpp/.cpp` | 把 `HAL_GPIO_WritePin/TogglePin` 收进来，业务层只调 `On/Off/Toggle` |

### 5.3 `App/` —— 应用层（只认接口）

| 文件 | 职责 | 优先级 / 栈 |
|---|---|---|
| `AppInit.cpp` | **组合根**：new 出所有对象、按顺序初始化、失败就返回错误码 | 跑在启动任务（2KB 栈） |
| `UartEchoTask.*` | 收帧 → 校验 → 回帧；队列接收 | 3 / 384 w |
| `CanEchoTask.*` | 收帧打印；回环自测每 5s 发一帧 | 2 / 320 w |
| `ServoTask.*` | 两个舵机反向扫描 | 1 / 320 w |
| `HeartbeatTask.*` | 等定时器信号量 → 翻灯 | 1 / 256 w |

---

## 6. `Drivers/` 和 `Middlewares/`

| 路径 | 是什么 | 看不看 |
|---|---|---|
| `Drivers/CMSIS/Include/core_cm3.h` | Cortex-M3 内核寄存器定义、`__disable_irq()` 等 | 查寄存器时看 |
| `Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f103xb.h` | 外设寄存器结构体（`TIM2`、`USART1` 都在这里定义） | 想知道寄存器地址时看 |
| `Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c` | UART 的 HAL 实现 | 想知道 `HAL_UART_Receive_DMA` 内部怎么走时看 |
| `Middlewares/.../Source/tasks.c` | FreeRTOS 调度器核心 | 讲"任务切换"时看 |
| `.../queue.c` | 队列实现 | — |
| `.../portable/GCC/ARM_CM3/port.c` | 移植层：PendSV/SVC、上下文切换汇编 | 讲中断优先级时看 |
| `.../portable/MemMang/heap_4.c` | 堆实现（首次适配 + 相邻块合并） | 讲"堆从哪来"时看 |
| `.../CMSIS_RTOS_V2/cmsis_os2.c` | CMSIS-RTOS2 封装（`osThreadNew` → `xTaskCreate`） | 看 `osKernelStart` 到底做了什么 |

---

## 7. `cmake/` 与 `build/`

| 文件 | 作用 |
|---|---|
| `cmake/gcc-arm-none-eabi.cmake` | 工具链文件：指定 `arm-none-eabi-gcc/g++`、编译选项、`-specs=nano.specs` 之类 |
| `cmake/stm32cubemx/CMakeLists.txt` | **CubeMX 生成**：列出 `Core/Src`、HAL 源文件、FreeRTOS 源文件 |
| `build/` | 构建产物（`.o`、`.elf`、`.map`、`compile_commands.json`），**已 gitignore，可随时删** |

`build/My_framework.map` 很有用：想看某个函数占多少 flash、某个符号被谁引用，都在里面。

---

## 8. `docs/` 各文档

| 文件 | 内容 |
|---|---|
| `Architecture.md` | 架构图、分层原则、目录树 |
| `Interface_and_Porting.md` | 接口契约、三种方式对照、移植八步 |
| `CubeMX_Config_Guide.md` | 每个 CubeMX 选项的含义（时钟树、DMA、CAN 位时序、FreeRTOS…） |
| `Step2_Layering_Refactor.md` | 重构记录 + 踩过的坑（栈溢出、堆、CAN 句柄名） |
| `Journey_Log.md` | 学习过程问答（可以当作答辩素材） |
| `File_Map.md` | 本文件 |

---

## 9. 速查：想改 X 去哪

| 想做的事 | 去改 |
|---|---|
| 加一个新外设（如 ADC） | CubeMX 配 → `Common/IAdc.hpp` → `Bsp/Stm32Adc.*` → `AppInit` 里 new |
| 换串口波特率 | `AppInit.cpp` 的 `kUartBaudrate`（或 CubeMX） |
| 改回显协议 | `Common/FrameCodec.hpp` + `App/UartEchoTask.cpp` |
| 改任务优先级/栈 | 对应 `App/*.cpp` 的 `xTaskCreate` |
| 改堆大小 | `FreeRTOSConfig.h` 的 `USER CODE BEGIN Defines` |
| 改 LED 引脚 | `AppInit.cpp` 的 `BoardLed led(...)` |
| 改 CAN 波特率 | `Core/Src/can.c` 的 `Prescaler/TimeSeg1/TimeSeg2` |
| 换芯片 | `Core/ Drivers/ startup/ ld/ cmake/` 整套替换 + `AppInit.cpp` 的句柄 |
| 加编译文件 | 根 `CMakeLists.txt` 的 `FRAMEWORK_SOURCES` |
