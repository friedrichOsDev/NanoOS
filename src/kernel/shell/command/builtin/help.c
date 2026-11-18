/**
 * @file help.c
 * @brief Built-in help command.
 * @author friedrichOsDev
 */

#include "../_command.h"

#include <shell/output.h>

static shell_command_info_t custom_commands[SHELL_MAX_CUSTOM_COMMANDS];

/** @brief Print one aligned command entry. */
static kerr_t shell_help_print_entry(const char *name, const char *description) {
    size_t name_length = 0U;
    while (name[name_length] != '\0') {
        name_length++;
    }

    K_TRY(shell_write("  "));
    K_TRY(shell_write(name));

    size_t padding = (name_length < 12U) ? (12U - name_length) : 1U;
    for (size_t i = 0U; i < padding; i++) {
        K_TRY(shell_write(" "));
    }

    K_TRY(shell_write(description));
    return shell_write_u32((uint32_t)'\n');
}

/** @brief List immutable built-ins and a lock-protected custom-command snapshot. */
kerr_t shell_cmd_help(int argc, const char *const argv[]) {
    (void)argv;

    if (argc != 1) {
        (void)shell_write_line("Usage: help");
        return E_INVAL;
    }

    K_TRY(shell_write_line("Available commands:"));

    for (size_t i = 0U; i < shell_builtin_count(); i++) {
        const shell_command_t *command = shell_builtin_at(i);
        if (command) {
            K_TRY(shell_help_print_entry(command->name, command->description));
        }
    }

    size_t custom_count = shell_command_registry_snapshot(
        custom_commands, SHELL_MAX_CUSTOM_COMMANDS);
    for (size_t i = 0U; i < custom_count; i++) {
        K_TRY(shell_help_print_entry(
            custom_commands[i].name, custom_commands[i].description));
    }

    return E_OK;
}
