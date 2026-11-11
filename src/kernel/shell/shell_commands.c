/**
 * @file shell_commands.c
 * @brief Built-in kernel shell commands and command dispatch.
 * @author friedrichOsDev
 */

#include "shell_internal.h"
#include <arch/arch.h>
#include <drivers/rtc.h>
#include <kernel/sys_def.h>
#include <lib/util/str.h>
#include <mm/memory.h>
#include <sched/process.h>
#include <sched/thread.h>

typedef struct {
    const char *name;
    const char *description;
} shell_command_info_t;

static const shell_command_info_t shell_commands[] = {
    {"help", "Display available commands."},
    {"clear", "Clear the terminal output."},
    {"echo", "Print the supplied arguments."},
    {"uname", "Display NanoOS system information."},
    {"uptime", "Display system uptime using the HPET."},
    {"meminfo", "Display physical memory usage."},
    {"psinfo", "List processes, threads and CPU cores."},
    {"time", "Display the current RTC-based system time."},
    {"shutdown", "Power off NanoOS using ACPI."}};

/**
 * @brief Writes an unsigned integer in decimal notation.
 * @param[in] value Number to print.
 */
static void shell_write_u64(uint64_t value) {
    char digits[21];
    size_t count = 0U;

    do {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U && count < sizeof(digits));

    while (count > 0U) {
        shell_writec((uint32_t)(uint8_t)digits[--count]);
    }
}

/**
 * @brief Writes a decimal value with leading zeroes.
 * @param[in] value Value to print.
 * @param[in] width Minimum number of digits.
 */
static void shell_write_padded(uint64_t value, size_t width) {
    char digits[21];
    size_t count = 0U;

    do {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U && count < sizeof(digits));

    while (count < width && count < sizeof(digits)) {
        digits[count++] = '0';
    }
    while (count > 0U) {
        shell_writec((uint32_t)(uint8_t)digits[--count]);
    }
}

/**
 * @brief Prints the command usage table.
 */
static void command_help(void) {
    shell_writeln("Available commands:");
    for (size_t i = 0U; i < (sizeof(shell_commands) / sizeof(shell_commands[0])); i++) {
        shell_write("  ");
        shell_write(shell_commands[i].name);
        shell_write(" - ");
        shell_writeln(shell_commands[i].description);
    }
}

/**
 * @brief Prints the elapsed HPET uptime.
 */
static void command_uptime(void) {
    uint64_t seconds = hpet_uptime_ms() / 1000U;
    uint64_t days = seconds / 86400U;
    uint64_t hours = (seconds / 3600U) % 24U;
    uint64_t minutes = (seconds / 60U) % 60U;
    seconds %= 60U;

    shell_write("Uptime: ");
    shell_write_u64(days);
    shell_write("d ");
    shell_write_u64(hours);
    shell_write("h ");
    shell_write_u64(minutes);
    shell_write("m ");
    shell_write_u64(seconds);
    shell_writeln("s");
}

/**
 * @brief Prints physical memory totals from the PMM page counters.
 */
static void command_meminfo(void) {
    const uint64_t page_size = 4096ULL;
    uint64_t total_mib = (pmm_get_total_pages() * page_size) / (1024U * 1024U);
    uint64_t used_mib = (pmm_get_used_pages() * page_size) / (1024U * 1024U);
    uint64_t free_mib = (pmm_get_free_pages() * page_size) / (1024U * 1024U);

    shell_writeln("Physical memory:");
    shell_write("  Total: ");
    shell_write_u64(total_mib);
    shell_writeln(" MiB");
    shell_write("  Used:  ");
    shell_write_u64(used_mib);
    shell_writeln(" MiB");
    shell_write("  Free:  ");
    shell_write_u64(free_mib);
    shell_writeln(" MiB");
}

/**
 * @brief Returns a readable name for a scheduler thread state.
 * @param[in] state Scheduler state.
 * @return Static state name.
 */
static const char *thread_state_name(thread_state_t state) {
    switch (state) {
    case THREAD_EMBRYO:
        return "EMBRYO";
    case THREAD_READY:
        return "READY";
    case THREAD_RUNNING:
        return "RUNNING";
    case THREAD_BLOCKED:
        return "BLOCKED";
    case THREAD_SLEEPING:
        return "SLEEPING";
    case THREAD_DEAD:
        return "DEAD";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Lists registered processes, their threads and the detected CPU core count.
 */
static void command_psinfo(void) {
    shell_writeln("Processes and threads:");

    spinlock_t *list_lock = proc_get_list_lock();
    irq_flags_t list_flags = spinlock_acquire_irqsave(list_lock);
    process_t *process = proc_get_list();

    while (process) {
        irq_flags_t process_flags = spinlock_acquire_irqsave(&process->lock);

        shell_write("PID ");
        shell_write_u64(process->pid);
        shell_write("  ");
        shell_write(process->name);
        shell_write(" (");
        shell_write_u64((uint64_t)process->thread_count);
        shell_writeln(" threads)");

        thread_t *thread = process->threads;
        while (thread) {
            shell_write("  TID ");
            shell_write_u64(thread->tid);
            shell_write("  ");
            shell_write(thread->name);
            shell_write(" [");
            shell_write(thread_state_name(thread->state));
            shell_writeln("]");
            thread = thread->proc_next;
        }

        spinlock_release_irqrestore(&process->lock, process_flags);
        process = process->next;
    }

    spinlock_release_irqrestore(list_lock, list_flags);

    shell_write("Available CPU cores: ");
    shell_write_u64((uint64_t)smp_get_cpu_count());
    shell_writeln("");
}

/**
 * @brief Prints the current system date and time.
 */
static void command_time(void) {
    rtc_time_t now = time_get_now();

    shell_write("Date: ");
    shell_write_padded(now.year, 4U);
    shell_write("-");
    shell_write_padded(now.month, 2U);
    shell_write("-");
    shell_write_padded(now.day, 2U);
    shell_write("  Time: ");
    shell_write_padded(now.hour, 2U);
    shell_write(":");
    shell_write_padded(now.minute, 2U);
    shell_write(":");
    shell_write_padded(now.second, 2U);
    shell_writeln("");
}

/**
 * @brief Executes one parsed shell command.
 * @param[in] line Null-terminated command line.
 */
void shell_execute_command(const char *line) {
    if (!line) {
        return;
    }

    while (*line == ' ' || *line == '\t') {
        line++;
    }
    if (*line == '\0') {
        return;
    }

    char command[32];
    size_t command_length = 0U;
    while (line[command_length] != '\0' &&
           line[command_length] != ' ' &&
           line[command_length] != '\t') {
        if (command_length + 1U >= sizeof(command)) {
            shell_writeln("Command name is too long.");
            return;
        }
        command[command_length] = line[command_length];
        command_length++;
    }
    command[command_length] = '\0';

    const char *arguments = line + command_length;
    while (*arguments == ' ' || *arguments == '\t') {
        arguments++;
    }

    if (strcmp(command, "help") == 0) {
        command_help();
    } else if (strcmp(command, "clear") == 0) {
        shell_clear();
    } else if (strcmp(command, "echo") == 0) {
        shell_writeln(arguments);
    } else if (strcmp(command, "uname") == 0) {
        shell_writeln("NanoOS x86_64");
    } else if (strcmp(command, "uptime") == 0) {
        command_uptime();
    } else if (strcmp(command, "meminfo") == 0) {
        command_meminfo();
    } else if (strcmp(command, "psinfo") == 0) {
        command_psinfo();
    } else if (strcmp(command, "time") == 0) {
        command_time();
    } else if (strcmp(command, "shutdown") == 0) {
        shell_writeln("Requesting ACPI shutdown...");
        acpi_power_off();
        shell_writeln("ACPI shutdown did not power off the system.");
    } else {
        shell_write("Unknown command: ");
        shell_writeln(command);
        shell_writeln("Type 'help' to see available commands.");
    }
}
