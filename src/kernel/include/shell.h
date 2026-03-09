/**
 * @file shell.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#define MAX_COMMAND_LENGTH 128
#define MAX_ARGUMENTS 16
#define MAX_COMMANDS 64
#define SHELL_PROMPT U"friedrichOsDev@NanoOS> "

typedef void (*shell_command_func_t)(int argc, uint32_t** argv);

typedef struct {
    const uint32_t* name;
    shell_command_func_t handler;
    const uint32_t* description;
} shell_command_t;

typedef struct {
    shell_command_t commands[MAX_COMMANDS];
    size_t command_count;
} command_list_t;

extern command_list_t commands;

void shell_init(void);
void shell_handle_input(uint32_t c);
void shell_register_command(const shell_command_t *command);