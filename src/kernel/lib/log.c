/**
 * @file log.c
 * @brief Logging implementation.
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <kernel/panic.h>
#include <kernel/sync.h>
#include <lib/log.h>

spinlock_t log_lock = SPINLOCK_INIT;

/**
 * @brief Log a format string with a specific log level.
 *
 * @param[in] level The log level.
 * @param[in] fmt The format string.
 * @param[in] ... Additional arguments corresponding to the format string.
 */
void k_log(log_level_t level, const char *format, ...) {
    const char *prefixes[] = {
        [LOG_LEVEL_TRACE] = "TRACE: ",
        [LOG_LEVEL_INFO] = "INFO: ",
        [LOG_LEVEL_WARN] = "WARN: ",
        [LOG_LEVEL_ERROR] = "ERROR: ",
        [LOG_LEVEL_PANIC] = "PANIC: ",
        [LOG_LEVEL_DEBUG] = "DEBUG: ",
        [LOG_LEVEL_ASSERT] = "ASSERT: "};

    va_list args;
    va_start(args, format);

    irq_flags_t flags = spinlock_acquire_irqsave(&log_lock);

    if (level >= LOG_LEVEL_TRACE && level <= LOG_LEVEL_ASSERT) {
        serial_puts(prefixes[level]);
    }
    vserial_printf(format, args);
    serial_puts("\n");

    spinlock_release_irqrestore(&log_lock, flags);

    if (level == LOG_LEVEL_PANIC || level == LOG_LEVEL_ASSERT) {
        va_end(args);
        panic();
    }

    va_end(args);
}
