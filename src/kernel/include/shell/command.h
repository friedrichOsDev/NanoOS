/**
 * @file command.h
 * @brief Public shell command registration interface.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

#define SHELL_COMMAND_NAME_MAX 32U
#define SHELL_COMMAND_DESCRIPTION_MAX 96U

/** @brief Callback used by built-in and registered shell commands. */
typedef kerr_t (*shell_command_fn_t)(int argc, const char *const argv[]);

/** @brief Immutable command descriptor supplied during registration. */
typedef struct {
    const char *name;
    const char *description;
    shell_command_fn_t execute;
} shell_command_t;

/**
 * @brief Register a custom kernel command.
 *
 * The registry copies the name and description. The callback must remain
 * valid until the command has been unregistered or the shell is deinitialized.
 *
 * @param[in] command Command descriptor.
 * @return E_OK on success, or an error code.
 */
kerr_t shell_command_register(const shell_command_t *command);

/**
 * @brief Unregister a custom kernel command.
 *
 * Built-in commands are reserved and cannot be unregistered.
 *
 * @param[in] name Command name.
 * @return E_OK on success, or an error code.
 */
kerr_t shell_command_unregister(const char *name);
