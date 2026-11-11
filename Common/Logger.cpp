#include "Logger.hpp"
#include <cstdio> //

void Logger::Info(const char* tag, const char* msg) {
    //
    printf("[INFO][%s] %s\r\n", tag, msg);
}

void Logger::Error(const char* tag, const char* msg, ErrorCode code) {
    //
    printf("[ERROR][%s], %s, Code: %s\r\n", tag, msg, ErrorCodeToString(code));
}