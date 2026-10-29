#include <lib/log.h>
#include <arch/arch.h>

void k_log(log_level_t level, const char *format, ...) {
    const char *prefixes[] = {
        [LOG_LEVEL_TRACE] = "TRACE: ",
        [LOG_LEVEL_INFO]  = "INFO: ",
        [LOG_LEVEL_WARN]  = "WARN: ",
        [LOG_LEVEL_ERROR] = "ERROR: ",
        [LOG_LEVEL_PANIC] = "PANIC: ",
        [LOG_LEVEL_DEBUG] = "DEBUG: ",
        [LOG_LEVEL_ASSERT] = "ASSERT: "
    };

    va_list args;
    va_start(args, format);

    if (level >= LOG_LEVEL_TRACE && level <= LOG_LEVEL_ASSERT) {
        serial_puts(prefixes[level]);
    }

    vserial_printf(format, args);

    serial_puts("\n");

    if (level == LOG_LEVEL_PANIC || level == LOG_LEVEL_ASSERT) {
        va_end(args);
        panic();
    }

    va_end(args);
}
