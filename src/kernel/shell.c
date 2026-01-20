/*
 * @file shell.c
 * @brief Kernel shell application
 * @author friedrichOsDev
 */

#include <shell.h>
#include <console.h>
#include <serial.h>
#include <string.h>
#include <heap.h>

char command_buffer[MAX_COMMAND_LENGTH];
size_t command_buffer_pos = 0;
command_list_t commands;

/*
 * Clear command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_clear(int argc, char** argv) {
    (void)argc;
    (void)argv;
    console_clear();
}

/*
 * Version command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_version(int argc, char** argv) {
    (void)argc;
    (void)argv;
    console_puts("Kernel Version 1.0.0\n");
}

/*
 * Reboot command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_reboot(int argc, char** argv) {
    (void)argc;
    (void)argv;
    console_puts("Rebooting system...\n");
    
    // for now a Triple fault will cause a reboot
    uint16_t empty_idt[3] = {0, 0, 0};
    __asm__ __volatile__ ("lidt %0" : : "m"(empty_idt));
}

/*
 * A function to initialize the shell
 * @param void
 */
void shell_init(void) {
    console_clear();
    console_puts(SHELL_PROMPT);

    commands.command_count = 0;
    command_buffer_pos = 0;
    command_buffer[0] = '\0';
    serial_puts("shell_init: initialized command buffer\n");

    shell_command_t clear_command = {
        .name = "clear",
        .handler = shell_command_clear,
        .description = "Clears the console screen"
    };
    shell_register_command(&clear_command);

    shell_command_t version_command = {
        .name = "version",
        .handler = shell_command_version,
        .description = "Displays the kernel version"
    };
    shell_register_command(&version_command);

    shell_command_t reboot_command = {
        .name = "reboot",
        .handler = shell_command_reboot,
        .description = "Reboots the system"
    };
    shell_register_command(&reboot_command);
}

/*
 * Handle a complete command
 * @param command: The command string to handle
 */
void shell_handle_command(const char* command) {
    if (command == NULL || command[0] == '\0') return;
    
    char* argv[MAX_ARGUMENTS];
    int argc = 0;

    char* cmd_copy = (char*)command;
    char* token = cmd_copy;

    while (*cmd_copy) {
        if (*cmd_copy == ' ') {
            *cmd_copy = '\0';
            if (token != cmd_copy && argc < MAX_ARGUMENTS) {
                argv[argc++] = token;
            }
            token = cmd_copy + 1;
        }
        cmd_copy++;
    }
    if (argc < MAX_ARGUMENTS && *token != '\0') {
        argv[argc++] = token;

    }
    
    if (argc == 0) return;

    for (size_t i = 0; i < commands.command_count; i++) {
        if (strcmp(argv[0], commands.commands[i].name) == 0) {
            commands.commands[i].handler(argc, argv);
            return;
        }
    }

    console_puts("Unknown command: ");
    console_puts(argv[0]);
    console_putc('\n');
}

/*
 * Register a new command
 * @param command: Pointer to the command structure of the command to register
 */
void shell_register_command(const shell_command_t *command) {
    if (!command) return;

    serial_puts("Shell: Registering command: ");
    serial_puts(command->name);
    serial_puts("\n");
    
    if (commands.command_count >= MAX_COMMANDS) {
        serial_puts("Shell: Too many commands registered!\n");
        return;
    }

    commands.commands[commands.command_count].name = command->name;
    commands.commands[commands.command_count].handler = command->handler;
    commands.commands[commands.command_count].description = command->description;
    commands.command_count++;
}

/*
 * Handle input character for the shell
 * @param c: The input character
 */
void shell_handle_input(char c) {
    if (c != (char)0) {
        if (c == '\b') {
            if (command_buffer_pos > 0) {
                command_buffer_pos--;
                console_putc(c);
            }
            return;
        } 
        
        if (c == '\n') {
            console_putc(c);
            command_buffer[command_buffer_pos] = '\0';

            shell_handle_command(command_buffer);

            command_buffer_pos = 0;
            command_buffer[0] = '\0';

            console_puts(SHELL_PROMPT);
            return;
        }

        if (command_buffer_pos < MAX_COMMAND_LENGTH - 1) {
            command_buffer[command_buffer_pos] = c;
            command_buffer_pos++;
            command_buffer[command_buffer_pos] = '\0';
            console_putc(c);
            return;
        }
    }   
}
