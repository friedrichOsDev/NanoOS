#pragma once

#include <pch.h>

#include <kernel/panic.h>

typedef enum {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_PANIC,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_ASSERT
} log_level_t;

void k_log(log_level_t level, const char *format, ...);

#define log_trace(fmt, ...) k_log(LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...) k_log(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...) k_log(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) k_log(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define log_panic(fmt, ...) k_log(LOG_LEVEL_PANIC, fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...) k_log(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define log_assert(fmt, ...) k_log(LOG_LEVEL_ASSERT, fmt, ##__VA_ARGS__)
