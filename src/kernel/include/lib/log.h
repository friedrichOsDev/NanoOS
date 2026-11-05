/**
 * @file log.h
 * @brief Logging macros with different log levels.
 * @author friedrichOsDev
 */

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"

#define log_trace(fmt, ...) k_log(LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...) k_log(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...) k_log(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) k_log(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define log_panic(fmt, ...) k_log(LOG_LEVEL_PANIC, fmt, ##__VA_ARGS__)
#define log_assert(fmt, ...) k_log(LOG_LEVEL_ASSERT, fmt, ##__VA_ARGS__)

#ifdef KERNEL_DEBUG_BUILD
#define log_debug(fmt, ...) k_log(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

static inline void _k_log_var_bool(const char *name, bool val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %s", name, val ? "true" : "false");
}
static inline void _k_log_var_char(const char *name, char val) {
    k_log(LOG_LEVEL_DEBUG, "%s = '%c'", name, val);
}
static inline void _k_log_var_schar(const char *name, signed char val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %d", name, (int)val);
}
static inline void _k_log_var_uchar(const char *name, unsigned char val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %u (%02x)", name, (unsigned)val, (unsigned)val);
}
static inline void _k_log_var_short(const char *name, short val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %d", name, (int)val);
}
static inline void _k_log_var_ushort(const char *name, unsigned short val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %u (%04x)", name, (unsigned)val, (unsigned)val);
}
static inline void _k_log_var_int(const char *name, int val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %d", name, val);
}
static inline void _k_log_var_uint(const char *name, unsigned int val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %u (%08x)", name, val, val);
}
static inline void _k_log_var_long(const char *name, long val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %ld", name, val);
}
static inline void _k_log_var_ulong(const char *name, unsigned long val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %lu (%016lx)", name, val, val);
}
static inline void _k_log_var_llong(const char *name, long long val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %lld", name, val);
}
static inline void _k_log_var_ullong(const char *name, unsigned long long val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %llu (%016llx)", name, val, val);
}
static inline void _k_log_var_float(const char *name, float val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %f", name, (double)val);
}
static inline void _k_log_var_double(const char *name, double val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %lf", name, val);
}
static inline void _k_log_var_ldouble(const char *name, long double val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %Lf", name, val);
}
static inline void _k_log_var_str(const char *name, char *val) {
    k_log(LOG_LEVEL_DEBUG, "%s = \"%s\"", name, val);
}
static inline void _k_log_var_cstr(const char *name, const char *val) {
    k_log(LOG_LEVEL_DEBUG, "%s = \"%s\"", name, val);
}
static inline void _k_log_var_ptr(const char *name, const void *val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %p", name, val);
}
static inline void _k_log_var_default(const char *name, uintptr_t val) {
    k_log(LOG_LEVEL_DEBUG, "%s = %p", name, (void *)val);
}

#define log_var(x)                                 \
    do {                                           \
        _Generic((x),                              \
            bool: _k_log_var_bool,                 \
            char: _k_log_var_char,                 \
            signed char: _k_log_var_schar,         \
            unsigned char: _k_log_var_uchar,       \
            short: _k_log_var_short,               \
            unsigned short: _k_log_var_ushort,     \
            int: _k_log_var_int,                   \
            unsigned int: _k_log_var_uint,         \
            long: _k_log_var_long,                 \
            unsigned long: _k_log_var_ulong,       \
            long long: _k_log_var_llong,           \
            unsigned long long: _k_log_var_ullong, \
            float: _k_log_var_float,               \
            double: _k_log_var_double,             \
            long double: _k_log_var_ldouble,       \
            char *: _k_log_var_str,                \
            const char *: _k_log_var_cstr,         \
            void *: _k_log_var_ptr,                \
            const void *: _k_log_var_ptr,          \
            default: _k_log_var_default)(#x, (x)); \
    } while (0)

#define log_registers(r)      \
    do {                      \
        log_var((r)->rax);    \
        log_var((r)->rbx);    \
        log_var((r)->rcx);    \
        log_var((r)->rdx);    \
        log_var((r)->rsi);    \
        log_var((r)->rdi);    \
        log_var((r)->rbp);    \
        log_var((r)->rsp);    \
        log_var((r)->r8);     \
        log_var((r)->r9);     \
        log_var((r)->r10);    \
        log_var((r)->r11);    \
        log_var((r)->r12);    \
        log_var((r)->r13);    \
        log_var((r)->r14);    \
        log_var((r)->r15);    \
        log_var((r)->rip);    \
        log_var((r)->cs);     \
        log_var((r)->rflags); \
    } while (0)
#else
static inline void _noop_log(const char *fmt, ...) { (void)fmt; }
#define log_debug(fmt, ...) _noop_log(fmt, ##__VA_ARGS__)
#define log_var(x) ((void)(x))
#define log_registers(r) ((void)(r))
#endif

#pragma GCC diagnostic pop
