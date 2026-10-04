#ifndef GENERIC_LOGGER_H
#define GENERIC_LOGGER_H

#include <Arduino.h>
#include <stdarg.h>

/**
 * @file GenericLogger.h
 * @brief Colored logging helpers and module log macros.
 */

/**
 * @brief Log severity levels.
 */
enum LogLevel
{
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
void printfLog(Stream &serial,
               const char *moduleName,
               LogLevel logLevel,
               size_t line,
               const char *fileName,
               const char *format,
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
void vprintfLog(Stream &serial,
                const char *moduleName,
                LogLevel logLevel,
                size_t line,
                const char *fileName,
                const char *format,
                va_list args);

/**
 * @brief Declares the output stream used by a module. Use once per .cpp file.
 *
 * Example: ASSIGN_LOG_MACROS(Robot, Serial);
 */
#define ASSIGN_LOG_MACROS(module, serial)                 \
    static inline Stream &_logSerial() { return serial; } \
    static inline const char *_logModule() { return #module; }

/**
 * @brief Log macros. __LINE__ and __FILE__ expand at the call site,
 *        so the correct source location is printed.
 *
 * Example: LOG_D(Robot, "Applied configuration");
 *          LOG_I(Robot, "throttle=%d", value);
 */
#define LOG_D(...) printfLog(_logSerial(), _logModule(), DEBUG, __LINE__, __FILE__, __VA_ARGS__)
#define LOG_I(...) printfLog(_logSerial(), _logModule(), INFO, __LINE__, __FILE__, __VA_ARGS__)
#define LOG_W(...) printfLog(_logSerial(), _logModule(), WARN, __LINE__, __FILE__, __VA_ARGS__)
#define LOG_E(...) printfLog(_logSerial(), _logModule(), ERROR, __LINE__, __FILE__, __VA_ARGS__)
#define LOG_F(...) printfLog(_logSerial(), _logModule(), FATAL, __LINE__, __FILE__, __VA_ARGS__)

#endif