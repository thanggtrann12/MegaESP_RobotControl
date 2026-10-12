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
 * @brief Khai báo module name cho file hiện tại và liên kết tới stream chung.
 * Dùng ở các file .cpp khác ngoài main.cpp.
 */
#define ASSIGN_LOG_MACROS(module)                              \
    static inline const char *_logModule() { return #module; } \
    extern Stream &_logSerial(); // Khai báo để file này biết stream chung nằm ở main.cpp

/**
 * @brief Khởi tạo stream đầu ra duy nhất cho toàn chương trình.
 * CHỈ dùng DUY NHẤT 1 lần trong main.cpp.
 * @param serial Output stream (ví dụ: Serial).
 */
#define INIT_LOG(serial) \
    Stream &_logSerial() { return serial; }

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