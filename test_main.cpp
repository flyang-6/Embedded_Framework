#include <iostream>
#include "Common/Logger.hpp"

int TestMain() {
    std::cout << "--- Framework Test Start ---" << std::endl;

    Logger::Info("System", "Framework initialized.");
    Logger::Error("UART1", "Init failed!", ErrorCode::ERR_INIT_FAIL);
    Logger::Error("CAN1", "Receive timeout!", ErrorCode::ERR_TIMEOUT);

    std::cout << "-- Framework Test End ---" << std::endl;
    return 0;
}
