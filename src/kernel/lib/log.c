#include <log.h>
#include <print.h>
#include <stdarg.h>

#define LOG_NO_COLOR 0xFFFFFF
#define LOG_INFO_COLOR 0x00FFFF
#define LOG_WARNING_COLOR 0xFFFF00
#define LOG_ERROR_COLOR 0xFF0000
#define LOG_DEBUG_COLOR 0x00FFFF
#define LOG_SUCCESS_COLOR 0x00FF00
#define LOG_FAILED_COLOR 0xFF0000
#define LOG_PANIC_COLOR 0xFF0000
#define LOG_EXCEPTION_COLOR 0xFF0000

void log_info(const char *format, ...) {
    printf("%k[INFO]%k: ", LOG_INFO_COLOR, LOG_NO_COLOR);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void log_warning(const char *format, ...) {
    printf("%k[WARNING]%k: ", LOG_WARNING_COLOR, LOG_NO_COLOR);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void log_error(const char *format, ...) {
    printf("%k[ERROR]%k: ", LOG_ERROR_COLOR, LOG_NO_COLOR);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void log_debug(const char *format, ...) {
    printf("%k[DEBUG]%k: ", LOG_DEBUG_COLOR, LOG_NO_COLOR);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void log_success(const char *format, ...) {
    printf("%k[SUCCESS]%k: ", LOG_SUCCESS_COLOR, LOG_NO_COLOR);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void log_failed(const char *format, ...) {
    printf("%k[FAILED]%k: ", LOG_FAILED_COLOR, LOG_NO_COLOR);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void log_kernel_panic(const char *format, ...) {
    printf("%k[KERNEL PANIC]%k: ", LOG_PANIC_COLOR, LOG_NO_COLOR);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void log_cpu_exception(uint32_t exception_code, const char *format, ...) {
    printf("%k[CPU EXCEPTION %x]%k: ", LOG_EXCEPTION_COLOR, exception_code, LOG_NO_COLOR);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}
