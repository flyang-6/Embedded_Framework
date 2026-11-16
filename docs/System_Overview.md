# 嵌入式系统架构与工作总结 (System Overview)

## 1. 项目背景
本项目基于 STM32F103C8T6 微控制器，采用现代 C++17 面向接口编程，结合 FreeRTOS 实时操作系统，搭建了一套高内聚、低耦合的嵌入式软件框架。工程使用 CMake 构建，通过 `pyocd` 进行烧录。

## 2. 核心设计思想 (Design Philosophy)
- **面向接口编程**：通过定义纯虚基类（`IUart`、`II2c`、`ICan`）作为硬件抽象层，使业务层（Application）完全与底层硬件（BSP）解耦。
- **分层架构**：
  - **Module 层**：`Common` 目录下的接口与通用组件（`Logger`、`ErrorCode`）。
  - **BSP 层**：具体硬件驱动（`Stm32Uart`、`Stm32I2c`），内部封装 HAL 库。
  - **Application 层**：业务逻辑与 FreeRTOS 任务调度（`UartTask`）。
- **错误处理机制**：全系统统一使用 `ErrorCode` 枚举类型。严禁“静默失败”，所有初始化、通信函数必须返回明确的错误码（如 `ERR_TIMEOUT`、`ERR_INIT_FAIL`）。

## 3. 核心模块解析 (Core Modules)

### 3.1 C/C++ 混编与启动流程
- **C 接口桥接**：在 `AppInit.cpp` 中通过 `extern "C"` 暴露 `App_Init()` 与 `User_Uart_RX_Callback()`，以便底层 C 语言代码（如 `main.c` 或中断处理）能够调用 C++ 类的成员函数。
- **系统启动**：`main.c` 完成 HAL 库与 FreeRTOS 初始化后，在 `freertos.c` 的默认任务中调用 `App_Init()` 并启动 DMA 接收。

### 3.2 UART 硬件驱动与中断处理
- **非阻塞设计**：串口接收采用 **DMA + 中断** 模式，避免 CPU 阻塞。
- **数据流转**：底层硬件收到数据后触发 DMA 中断，执行 `HAL_UART_RxCpltCallback` 回调，将数据通过 `User_Uart_RX_Callback` 压入 FreeRTOS 的队列中，供 `UartTask` 异步处理。

### 3.3 FreeRTOS 任务与队列 (UartTask)
- **任务划分**：`UartTask` 作为一个独立的 FreeRTOS 任务，通过 `xQueueReceive` 阻塞等待队列数据。
- **数据回显**：当任务从队列中取到数据后，调用 `Stm32Uart::Transmit()` 将数据原样发回，实现“串口回环测试”。

## 4. 踩坑记录与知识总结 (Troubleshooting & Knowledge)

### 4.1 常见问题解决
- **CubeMX 覆盖代码**：每次点击 `GENERATE CODE`，务必检查 `main.c` 的 `USER CODE` 区域，确认 `App_Init()` 和中断回调未被清空。
- **CMake 编译错误**：若出现 `Cannot find source file`，检查 `CMakeLists.txt` 中 `target_sources` 下的路径拼写（如斜杠 `/` 写成了点 `.`）。
- **语法错误**：C++ 编译器严格区分大小写，所有函数名、类名（如 `Start` 与 `start`）必须统一。头文件缺失 `private` 成员声明会导致 `.cpp` 初始化列表报错。

### 4.2 核心语法知识
- **`extern "C"`**：防止 C++ 名称修饰（Name Mangling），实现 C/C++ 交叉编译。
- **`enum class`**：强类型枚举，避免与传统 C 枚举发生命名冲突。
- **FreeRTOS 中断安全**：中断服务函数（ISR）中必须使用带 `FromISR` 后缀的 API（如 `xQueueSendFromISR`），并配合 `portYIELD_FROM_ISR` 触发任务切换。

## 5. 当前进度与后续计划
- **已实现**：底层 UART 驱动封装、Logger 日志系统、错误码机制、DMA+中断接收、FreeRTOS 队列回显。
- **待验证/完善**：I2C 底层驱动实现（`Stm32I2c` 目前仅完成写操作，读操作待补全）、CAN 接口的具体实现。