/**
 * @file shell_commands.h
 * @brief Builtin shell commands.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

typedef void (*cmd_handler_t)(uint64_t argc, const uint32_t **argv);

typedef struct {
    const uint32_t *name;
    uint64_t argc;
    cmd_handler_t cmd_handler;
} shell_cmd_t;
