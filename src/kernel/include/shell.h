/*
 * @file shell.h
 * @brief Header file for kernel shell application
 * @author friedrichOsDev
 */

#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stddef.h>

#define MAX_COMMAND_LENGTH 128
#define MAX_ARGUMENTS 16
#define MAX_COMMANDS 64
#define SHELL_PROMPT "> "

/*
 * Type definition for shell command handler function pointer
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
typedef void (*shell_command_func_t)(int argc, char** argv);

/*
 * Structure for a shell command
 */
typedef struct {
    const char* name;
    shell_command_func_t handler;
    const char* description;
} shell_command_t;

/*
 * Structure for command list
 */
typedef struct {
    shell_command_t commands[MAX_COMMANDS];
    size_t command_count;
} command_list_t;

extern command_list_t commands;

void shell_init(void);
void shell_handle_input(char c);
void shell_register_command(const shell_command_t *command);

#endif // SHELL_H