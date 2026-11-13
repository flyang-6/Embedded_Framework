#include "main.h"
#include "usart.h"
#include "Stm32Uart.hpp"
#include "UartTask.hpp"

// Global pointer for C language main.c to call
static UartTask* g_uart_task = nullptr;

extern "C" void App_Init(void) {
    // Instantiate hardware UART
    static Stm32Uart uart1(&huart1);
    // Instantiate task with the interface
    static UartTask uart_task(&uart1);

    // Store the pointer globally for the ISA callback
    g_uart_task = &uart_task;

    // Init the UART handware
    if (uart1.Init(115200) == ErrorCode::OK) {
        // Start the FreeRTOS task
        uart_task.Start();
    }
}

// c interface exposed for the c language ISR
extern "C" void User_Uart_RX_Callback(uint8_t* data, uint16_t len) {
    if (g_uart_task != nullptr) {
        // Push data into the FreeRTOS queue from ISR
        g_uart_task->PushDataFromISR(data, len);
    }
}