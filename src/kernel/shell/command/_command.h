/**
 * @file _command.h
 * @brief Internal shell command subsystem definitions.
 * @author friedrichOsDev
 */
#pragma once
#include <kernel/sync.h>
#include <shell/command.h>
#include "parser.h"

#define SHELL_MAX_CUSTOM_COMMANDS 48U
#define SHELL_COMMAND_QUEUE_SIZE 4U
#define SHELL_COMMAND_WORKER_SLEEP_MS 1U
#define SHELL_BUILTIN_COUNT 3U

typedef struct {
    char name[SHELL_COMMAND_NAME_MAX];
    char description[SHELL_COMMAND_DESCRIPTION_MAX];
} shell_command_info_t;

kerr_t shell_command_init(void);
kerr_t shell_command_deinit(void);
bool shell_command_is_stopped(void);
kerr_t shell_command_submit(const uint32_t *input, size_t length);
kerr_t shell_command_execute(int argc, const char *const argv[]);
kerr_t shell_command_registry_init(void);
void shell_command_registry_deinit(void);
bool shell_command_registry_lookup(const char *name, shell_command_fn_t *out_execute);
size_t shell_command_registry_snapshot(shell_command_info_t *out, size_t capacity);
const shell_command_t *shell_builtin_find(const char *name);
size_t shell_builtin_count(void);
const shell_command_t *shell_builtin_at(size_t index);
kerr_t shell_cmd_help(int argc, const char *const argv[]);
kerr_t shell_cmd_clear(int argc, const char *const argv[]);
kerr_t shell_cmd_echo(int argc, const char *const argv[]);
