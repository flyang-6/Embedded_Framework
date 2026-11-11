#pragma once
#include "ErrorCode.hpp"

//
class Logger {
public:
    //
    static void Info(const char* tag, const char* msg);

    //
    static void Error(const char*tag, const char* msg, ErrorCode code);
};