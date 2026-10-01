#ifndef GENERIC_LOGGER_H
#define GENERIC_LOGGER_H

#include <Arduino.h>
#include <stdarg.h>

/**
 * @file GenericLogger.h
 * @brief Colored logging helpers and module log macro generator.
 */

/**
 * @brief Log severity levels.
 */
enum LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

/**
 * @brief Prints a formatted log line with module metadata.
 * @param serial Output stream.
 * @param moduleName Module tag.
 * @param logLevel Severity level.
 * @param line Source line number.
 * @param fileName Source file name.
 * @param format printf-style format string.
 * @param ... Format arguments.
 */
void printfLog(Stream& serial,
               const char* moduleName,
               LogLevel logLevel,
               size_t line,
               const char* fileName,
               const char* format,
               ...);

/**
 * @brief va_list variant of printfLog.
 * @param serial Output stream.
 * @param moduleName Module tag.
 * @param logLevel Severity level.
 * @param line Source line number.
 * @param fileName Source file name.
 * @param format printf-style format string.
 * @param args Variable argument list.
 */
void vprintfLog(Stream& serial,
                const char* moduleName,
                LogLevel logLevel,
                size_t line,
                const char* fileName,
                const char* format,
                va_list args);

#define ASSIGN_LOG_MACROS(prefix, serial)                              \
    void prefix##_LogD(const char* input, ...)                         \
    {                                                                  \
        va_list args;                                                  \
        va_start(args, input);                                         \
        vprintfLog(serial, #prefix, DEBUG, __LINE__, __FILE__, input, args); \
        va_end(args);                                                  \
    }                                                                  \
    void prefix##_LogI(const char* input, ...)                         \
    {                                                                  \
        va_list args;                                                  \
        va_start(args, input);                                         \
        vprintfLog(serial, #prefix, INFO, __LINE__, __FILE__, input, args);  \
        va_end(args);                                                  \
    }                                                                  \
    void prefix##_LogW(const char* input, ...)                         \
    {                                                                  \
        va_list args;                                                  \
        va_start(args, input);                                         \
        vprintfLog(serial, #prefix, WARN, __LINE__, __FILE__, input, args);  \
        va_end(args);                                                  \
    }                                                                  \
    void prefix##_LogE(const char* input, ...)                         \
    {                                                                  \
        va_list args;                                                  \
        va_start(args, input);                                         \
        vprintfLog(serial, #prefix, ERROR, __LINE__, __FILE__, input, args); \
        va_end(args);                                                  \
    }                                                                  \
    void prefix##_LogF(const char* input, ...)                         \
    {                                                                  \
        va_list args;                                                  \
        va_start(args, input);                                         \
        vprintfLog(serial, #prefix, FATAL, __LINE__, __FILE__, input, args); \
        va_end(args);                                                  \
    }

#endif