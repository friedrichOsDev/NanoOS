#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stddef.h>

#define MAX_COMMAND_LENGTH 128
#define MAX_ARGUMENTS 16
#define MAX_COMMANDS 64

typedef void (*shell_command_func_t)(int argc, char** argv);

typedef struct {
    const char* name;
    shell_command_func_t handler;
    const char* description;
} shell_command_t;

typedef struct {
    shell_command_t commands[MAX_COMMANDS];
    size_t command_count;
} command_list_t;

extern command_list_t commands;

void shell_init(void);
void shell_handle_input(char c);
void shell_register_command(const shell_command_t *command);

#endif // SHELL_H