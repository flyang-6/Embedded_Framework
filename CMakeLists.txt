cmake_minimum_required(VERSION 3.22)

# ---- 语言标准 ----
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME My_framework)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C CXX ASM)

add_executable(${CMAKE_PROJECT_NAME})

# STM32CubeMX 生成的 Core / Drivers / FreeRTOS
add_subdirectory(cmake/stm32cubemx)

# ---- 三层的头文件路径 ----
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Common      # Module 层：接口、错误码、日志
    ${CMAKE_CURRENT_SOURCE_DIR}/Bsp         # BSP 层：具体芯片驱动
    ${CMAKE_CURRENT_SOURCE_DIR}/App         # Application 层：任务与业务
)

# ---- 框架源码 ----
set(FRAMEWORK_SOURCES
    Common/Logger.cpp
    Common/Servo.cpp
    Bsp/Stm32Uart.cpp
    Bsp/Stm32I2c.cpp
    Bsp/BoardLed.cpp
    App/AppInit.cpp
    App/UartEchoTask.cpp
    App/CanEchoTask.cpp
    App/HeartbeatTask.cpp
    App/ServoTask.cpp
)

# CAN / TIM 驱动只有在 CubeMX 生成对应头文件后才参与编译，
# 这样“先跑通串口”和“后加 CAN/TIM”两个阶段都能编过。
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/can.h")
    list(APPEND FRAMEWORK_SOURCES Bsp/Stm32Can.cpp)
    message(STATUS "CAN driver enabled (Core/Inc/can.h found)")
else()
    message(STATUS "CAN driver disabled (Core/Inc/can.h not found)")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/tim.h")
    list(APPEND FRAMEWORK_SOURCES Bsp/Stm32Timer.cpp)
    list(APPEND FRAMEWORK_SOURCES Bsp/Stm32Pwm.cpp)
    message(STATUS "TIM driver enabled (Core/Inc/tim.h found)")
else()
    message(STATUS "TIM driver disabled (Core/Inc/tim.h not found)")
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${FRAMEWORK_SOURCES})

# ---- 嵌入式 C++：关掉异常、RTTI、静态变量线程安全护栏，省 flash ----
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
)

# 使用 C++ 时去掉错误的 libob.a 依赖
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

target_link_libraries(${CMAKE_PROJECT_NAME} stm32cubemx)
