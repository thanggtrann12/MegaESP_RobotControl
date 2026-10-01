/**
 * @file GenericLogger.cpp
 * @brief Generic colored logger implementation.
 */

#include "GenericLogger.h"

namespace {
const char* GetLevelName(LogLevel logLevel) {
    switch (logLevel) {
        case DEBUG: return "DEBUG";
        case INFO:  return "INFO";
        case WARN:  return "WARN";
        case ERROR: return "ERROR";
        case FATAL: return "FATAL";
        default:    return "UNKNOWN";
    }
}

const char* GetLevelColor(LogLevel logLevel) {
    switch (logLevel) {
        case DEBUG: return "\033[36m";
        case INFO:  return "\033[32m";
        case WARN:  return "\033[33m";
        case ERROR: return "\033[31m";
        case FATAL: return "\033[91m";
        default:    return "\033[0m";
    }
}
}

void printfLog(Stream& serial,
               const char* moduleName,
               LogLevel logLevel,
               size_t line,
               const char* fileName,
               const char* format,
               ...) {
    va_list args;
    va_start(args, format);
    vprintfLog(serial, moduleName, logLevel, line, fileName, format, args);
    va_end(args);
}

void vprintfLog(Stream& serial,
                const char* moduleName,
                LogLevel logLevel,
                size_t line,
                const char* fileName,
                const char* format,
                va_list args) {
    char message[128];
    vsnprintf(message, sizeof(message), format, args);

    serial.print(GetLevelColor(logLevel));
    serial.print('[');
    serial.print(GetLevelName(logLevel));
    serial.print("][");
    serial.print(moduleName);
    serial.print("] ");
    serial.print(fileName);
    serial.print(':');
    serial.print(line);
    serial.print(" - ");
    serial.println(message);
    serial.print("\033[0m");
}