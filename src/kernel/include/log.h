/*
 * @file log.h
 * @brief Header file for logging functions implementation
 * @author friedrichOsDev
 */

#ifndef LOG_H
#define LOG_H

#include <stdint.h>

void log_info(const char *format, ...);
void log_warning(const char *format, ...);
void log_error(const char *format, ...);
void log_debug(const char *format, ...);
void log_success(const char *format, ...);
void log_failed(const char *format, ...);
void log_kernel_panic(const char *format, ...);
void log_cpu_exception(uint32_t exception_code, const char *format, ...);

#endif // LOG_H