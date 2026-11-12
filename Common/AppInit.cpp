#include "main.h"
//#include "Stm32Uart.hpp"
#include "UartTask.hpp"

//
extern "C" void App_Init(void) {
    //
    //
    static UartTask uart_task(nullptr);

    //
    uart_task.Start();
}