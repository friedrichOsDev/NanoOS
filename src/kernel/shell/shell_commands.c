/**
 * @file shell_commands.c
 * @brief Built-in NanoOS shell command implementation.
 * @author friedrichOsDev
 */

#include "shell_internal.h"
#include <arch/arch.h>
#include <drivers/rtc.h>
#include <mm/memory.h>
#include <sched/process.h>
#include <sched/thread.h>
#include <lib/util/str.h>

/** @brief Prints the supported built-in commands. */
static void command_help(void) {
    shell_writeln("Available commands:");
    shell_writeln("  help      - Show this help");
    shell_writeln("  clear     - Clear the terminal");
    shell_writeln("  echo      - Print arguments");
    shell_writeln("  uname     - Show operating system information");
    shell_writeln("  uptime    - Show system uptime");
    shell_writeln("  meminfo   - Show physical memory statistics");
    shell_writeln("  psinfo    - List processes, threads and CPU cores");
    shell_writeln("  time      - Show current RTC-based date and time");
    shell_writeln("  shutdown  - Power off through ACPI");
}

/** @brief Prints command arguments separated by one space. */
static void command_echo(const char *args) {
    shell_writeln(args);
}

/** @brief Displays NanoOS identity and architecture. */
static void command_uname(void) {
    shell_writeln("NanoOS 0.0.3 x86_64");
}

/** @brief Displays elapsed time since the HPET counter started. */
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

/** @brief Displays physical memory totals from the PMM page counters. */
static void command_meminfo(void) {
    const uint64_t page_size = 4096U;
    uint64_t total = pmm_get_total_pages() * page_size;
    uint64_t used = pmm_get_used_pages() * page_size;
    uint64_t free = pmm_get_free_pages() * page_size;

    shell_writeln("Physical memory (bytes):");
    shell_write("  Total: ");
    shell_write_u64(total);
    shell_writeln("");
    shell_write("  Used:  ");
    shell_write_u64(used);
    shell_writeln("");
    shell_write("  Free:  ");
    shell_write_u64(free);
    shell_writeln("");
}

/** @brief Lists all registered processes and their threads. */
static void command_psinfo(void) {
    uint64_t cpu_count = (uint64_t)smp_get_cpu_count();
    shell_write("Available CPU cores: ");
    for (uint64_t cpu = 0U; cpu < cpu_count; cpu++) {
        if (cpu > 0U) shell_write(" ");
        shell_write_u64(cpu);
    }
    shell_writeln("");
    shell_writeln("Processes:");

    spinlock_t *list_lock = proc_get_list_lock();
    irq_flags_t flags = spinlock_acquire_irqsave(list_lock);
    process_t *proc = proc_get_list();

    while (proc != NULL) {
        shell_write("PID ");
        shell_write_u64(proc->pid);
        shell_write("  ");
        shell_writeln(proc->name);

        irq_flags_t proc_flags = spinlock_acquire_irqsave(&proc->lock);
        thread_t *thread = proc->threads;
        while (thread != NULL) {
            shell_write("  TID ");
            shell_write_u64(thread->tid);
            shell_write("  ");
            shell_writeln(thread->name);
            thread = thread->proc_next;
        }
        spinlock_release_irqrestore(&proc->lock, proc_flags);

        proc = proc->next;
    }

    spinlock_release_irqrestore(list_lock, flags);
}

/** @brief Displays the current date and time derived from the RTC and HPET. */
static void command_time(void) {
    rtc_time_t now = time_get_now();

    shell_write("Date: ");
    shell_write_u64(now.year);
    shell_write("-");
    if (now.month < 10U) shell_write("0");
    shell_write_u64(now.month);
    shell_write("-");
    if (now.day < 10U) shell_write("0");
    shell_write_u64(now.day);
    shell_writeln("");

    shell_write("Time: ");
    if (now.hour < 10U) shell_write("0");
    shell_write_u64(now.hour);
    shell_write(":");
    if (now.minute < 10U) shell_write("0");
    shell_write_u64(now.minute);
    shell_write(":");
    if (now.second < 10U) shell_write("0");
    shell_write_u64(now.second);
    shell_writeln(" UTC");
}

/**
 * @brief Executes one parsed command line.
 * @param[in] line Null-terminated command line.
 */
void shell_execute_command(const char *line) {
    if (!line || line[0] == '\0') {
        return;
    }

    char command[SHELL_INPUT_MAX];
    size_t length = strlen(line);
    if (length >= sizeof(command)) {
        length = sizeof(command) - 1U;
    }
    for (size_t i = 0U; i < length; i++) command[i] = line[i];
    command[length] = '\0';

    char *cursor = command;
    while (*cursor == ' ' || *cursor == '\t') cursor++;
    char *name = cursor;
    while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t') cursor++;

    char *args = cursor;
    if (*cursor != '\0') {
        *cursor++ = '\0';
        while (*cursor == ' ' || *cursor == '\t') cursor++;
        args = cursor;
    } else {
        args = cursor;
    }

    if (strcmp(name, "help") == 0) {
        command_help();
    } else if (strcmp(name, "clear") == 0) {
        shell_clear();
    } else if (strcmp(name, "echo") == 0) {
        command_echo(args);
    } else if (strcmp(name, "uname") == 0) {
        command_uname();
    } else if (strcmp(name, "uptime") == 0) {
        command_uptime();
    } else if (strcmp(name, "meminfo") == 0) {
        command_meminfo();
    } else if (strcmp(name, "psinfo") == 0) {
        command_psinfo();
    } else if (strcmp(name, "time") == 0) {
        command_time();
    } else if (strcmp(name, "shutdown") == 0) {
        shell_writeln("Shutting down NanoOS...");
        shell_request_shutdown();
    } else {
        shell_write("Unknown command: ");
        shell_writeln(name);
        shell_writeln("Type 'help' to see available commands.");
    }
}
