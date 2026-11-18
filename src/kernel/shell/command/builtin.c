/**
 * @file builtin.c
 * @brief Immutable table of built-in shell commands.
 * @author friedrichOsDev
 */

#include "_command.h"

#include <lib/util/str.h>

static const shell_command_t builtin_commands[SHELL_BUILTIN_COUNT] = {
    {
        .name = "help",
        .description = "List available commands",
        .execute = shell_cmd_help,
    },
    {
        .name = "clear",
        .description = "Clear shell output",
        .execute = shell_cmd_clear,
    },
    {
        .name = "echo",
        .description = "Print arguments",
        .execute = shell_cmd_echo,
    },
};

/** @brief Find a built-in command by its case-sensitive name. */
const shell_command_t *shell_builtin_find(const char *name) {
    if (!name) {
        return NULL;
    }

    for (size_t i = 0U; i < SHELL_BUILTIN_COUNT; i++) {
        if (strcmp(builtin_commands[i].name, name) == 0) {
            return &builtin_commands[i];
        }
    }

    return NULL;
}

/** @brief Return the number of built-in commands. */
size_t shell_builtin_count(void) {
    return SHELL_BUILTIN_COUNT;
}

/** @brief Return a built-in command by its zero-based index. */
const shell_command_t *shell_builtin_at(size_t index) {
    if (index >= SHELL_BUILTIN_COUNT) {
        return NULL;
    }
    return &builtin_commands[index];
}
