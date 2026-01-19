#include <shell.h>
#include <console.h>
#include <serial.h>
#include <string.h>
#include <heap.h>

char command_buffer[MAX_COMMAND_LENGTH];
size_t command_buffer_pos = 0;
command_list_t commands;

void shell_command_clear(int argc, char** argv) {
    (void)argc;
    (void)argv;
    serial_puts("Executing clear command\n");
    console_clear();
}

void shell_command_version(int argc, char** argv) {
    (void)argc;
    (void)argv;
    serial_puts("Executing version command\n");
    console_puts("Kernel Version 1.0.0\n");
}

void shell_command_reboot(int argc, char** argv) {
    (void)argc;
    (void)argv;
    serial_puts("Executing reboot command\n");
    console_puts("Rebooting system...\n");
    
    // for now a Triple fault will cause a reboot
    uint16_t empty_idt[3] = {0, 0, 0};
    __asm__ __volatile__ ("lidt %0" : : "m"(empty_idt));
}

void shell_init(void) {
    console_clear();
    console_puts("> ");

    commands.command_count = 0;
    command_buffer_pos = 0;
    command_buffer[0] = '\0';
    serial_puts("shell_init: initialized command buffer\n");

    // Register default commands
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

    serial_puts("shell_init: registered default commands\n");
}

void shell_handle_command(const char* command) {
    if (command == NULL || command[0] == '\0') return;
    
    // split command into args
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
    serial_puts("Shell: Command registered successfully\n");
}

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

            // reset command buffer
            command_buffer_pos = 0;
            command_buffer[0] = '\0';

            console_puts("> ");
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