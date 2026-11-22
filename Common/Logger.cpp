#include "Logger.hpp"
#include "IUart.hpp"

#include <cstdarg>
#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

namespace {
IUart* g_port = nullptr;
SemaphoreHandle_t g_lock = nullptr;

void Output(const char* line, int len) {
    if (g_port == nullptr || len <= 0) {
        return;
    }

    bool locked = false;
    if (g_lock != nullptr && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        locked = (xSemaphoreTake(g_lock, pdMS_TO_TICKS(50)) == pdTRUE);
    }

    (void)g_port->Transmit(reinterpret_cast<const uint8_t*>(line),
                           static_cast<uint16_t>(len), 100U);

    if (locked) {
        (void)xSemaphoreGive(g_lock);
    }
}
}  // namespace

void Logger::Init(IUart* port) {
    g_port = port;
    if (g_lock == nullptr) {
        g_lock = xSemaphoreCreateMutex();
    }
}

void Logger::Info(const char* tag, const char* fmt, ...) {
    char body[96] = {0};
    va_list ap;
    va_start(ap, fmt);
    (void)vsnprintf(body, sizeof(body), fmt, ap);
    va_end(ap);

    char line[128];
    int n = snprintf(line, sizeof(line), "[INFO ][%s] %s\r\n", tag ? tag : "-", body);
    if (n > static_cast<int>(sizeof(line)) - 1) {
        n = static_cast<int>(sizeof(line)) - 1;
    }
    Output(line, n);
}

void Logger::Error(const char* tag, ErrorCode code, const char* fmt, ...) {
    char body[96] = {0};
    va_list ap;
    va_start(ap, fmt);
    (void)vsnprintf(body, sizeof(body), fmt, ap);
    va_end(ap);

    char line[128];
    int n = snprintf(line, sizeof(line), "[ERROR][%s] %s (code=%s)\r\n",
                     tag ? tag : "-", body, ErrorCodeToString(code));
    if (n > static_cast<int>(sizeof(line)) - 1) {
        n = static_cast<int>(sizeof(line)) - 1;
    }
    Output(line, n);
}
