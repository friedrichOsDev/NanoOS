/**
 * @file shell.c
 * @author friedrichOsDev
 */

#include <shell.h>
#include <console.h>
#include <string.h>
#include <serial.h>
#include <keyboard.h>
#include <kernel.h>
#include <rtc.h>
#include <heap.h>
#include <print.h>
#include <convert.h>

uint32_t command_buffer[MAX_COMMAND_LENGTH];
size_t command_buffer_pos = 0;
size_t cursor_pos = 0;
command_list_t commands;

void shell_command_clear(int argc, uint32_t** argv) {
    (void)argc;
    (void)argv;
    console_clear();
}

void shell_command_welcome(int argc, uint32_t** argv) {
    (void)argc;
    (void)argv;
    const uint32_t* welcome_window = 
    U"\n"
    U"  ┌──────────────────────────────┐\n"
    U"  │         Welcome to           │\n"
    U"  │   _  _                       │\n"
    U"  │  | \\| |__ _ _ _  ___         │\n"
    U"  │  | .` / _` | ' \\/ _ \\        │\n"
    U"  │  |_|\\_\\__,_|_||_\\___/  os    │\n"
    U"  │                              │\n"
    U"  └──────────────────────────────┘\n"
    U"\n";
    console_puts(welcome_window);
}

void shell_command_time(int argc, uint32_t** argv) {
    (void)argc;
    (void)argv;
    const char* time_str = rtc_get_time_format(1);
    while (*time_str) {
        console_putc((uint32_t)*time_str++);
    }
    console_putc(U'\n');
}

void shell_command_heap(int argc, uint32_t** argv) {
    (void)argc;
    (void)argv;
    
    heap_block_t* current = heap_get_list();
    char buf[128];

    console_puts(U"Heap Layout:\n");
    console_puts(U"Address    | Size (Bytes) | Status\n");
    console_puts(U"-----------|--------------|-----------\n");

    while (current) {
        const char* status = (current->magic == HEAP_MAGIC_FREE) ? "FREE" : "USED";
        snprintf(buf, sizeof(buf), "%08x | %-12d | %s\n", (uint32_t)current, (uint32_t)current->size, status);
        
        for (int i = 0; buf[i] != '\0'; i++) {
            console_putc((uint32_t)buf[i]);
        }
        current = current->next;
    }
}

bool shell_move_cursor_left(bool release) {
    if (release) return true;
    if (cursor_pos > 0) {
        console_move_cursor_left(false);
        serial_printf("Shell: Cursor moved left, pos: %d\n", cursor_pos);
        serial_printf("Shell: Command buffer pos: %d\n", command_buffer_pos);
        cursor_pos--;
    }
    return true;
}

bool shell_move_cursor_right(bool release) {
    if (release) return true;
    if (cursor_pos < command_buffer_pos) {
        console_move_cursor_right(false);
        serial_printf("Shell: Cursor moved right, pos: %d\n", cursor_pos);
        serial_printf("Shell: Command buffer pos: %d\n", command_buffer_pos);
        cursor_pos++;
    }
    return true;
}

void shell_init(void) {
    console_clear();
    console_puts(SHELL_PROMPT);

    commands.command_count = 0;
    command_buffer_pos = 0;
    cursor_pos = 0;
    command_buffer[0] = '\0';

    shell_command_t clear_command = {
        .name = U"clear",
        .handler = shell_command_clear,
        .description = U"Clears the console screen"
    };
    shell_register_command(&clear_command);

    shell_command_t welcome_command = {
        .name = U"welcome",
        .handler = shell_command_welcome,
        .description = U"Displays the welcome message"
    };
    shell_register_command(&welcome_command);

    shell_command_t time_command = {
        .name = U"time",
        .handler = shell_command_time,
        .description = U"Displays the current system time"
    };
    shell_register_command(&time_command);

    shell_command_t heap_command = {
        .name = U"heap",
        .handler = shell_command_heap,
        .description = U"Displays the current heap layout"
    };
    shell_register_command(&heap_command);

    keyboard_map_function_to_vk(VK_LEFT, shell_move_cursor_left);
    keyboard_map_function_to_vk(VK_RIGHT, shell_move_cursor_right);

    init_state = INIT_SHELL;
}

void shell_handle_command(const uint32_t* command) {
    if (command == NULL || command[0] == U'\0') return;
    
    uint32_t* argv[MAX_ARGUMENTS];
    int argc = 0;

    uint32_t* cmd_copy = (uint32_t*)command;
    uint32_t* token = cmd_copy;

    while (*cmd_copy) {
        if (*cmd_copy == U' ') {
            *cmd_copy = U'\0';
            if (token != cmd_copy && argc < MAX_ARGUMENTS) {
                argv[argc++] = token;
            }
            token = cmd_copy + 1;
        }
        cmd_copy++;
    }
    if (argc < MAX_ARGUMENTS && *token != U'\0') {
        argv[argc++] = token;

    }
    
    if (argc == 0) return;

    for (size_t i = 0; i < commands.command_count; i++) {
        if (u32_strcmp(argv[0], commands.commands[i].name) == 0) {
            commands.commands[i].handler(argc, argv);
            return;
        }
    }

    console_puts(U"Unknown command: ");
    console_puts(argv[0]);
    console_putc(U'\n');
}

void shell_handle_input(uint32_t c) {
    if (c != 0) {
        if (c == U'\b') {
            if (cursor_pos > 0) {
                memmove(&command_buffer[cursor_pos - 1], &command_buffer[cursor_pos], (command_buffer_pos - cursor_pos) * sizeof(uint32_t));
                command_buffer_pos--;
                cursor_pos--;
                command_buffer[command_buffer_pos] = U'\0';
                
                console_move_cursor_left(false);
                console_puts(&command_buffer[cursor_pos]);
                console_putc(U' ');

                for (size_t i = 0; i < (command_buffer_pos - cursor_pos) + 1; i++) {
                    console_move_cursor_left(false);
                }
            }
            return;
        } 
        
        if (c == U'\n') {
            console_putc(c);
            command_buffer[command_buffer_pos] = U'\0';
            
            uint32_t temp_buffer[MAX_COMMAND_LENGTH];
            u32_strcpy(temp_buffer, command_buffer);
            shell_handle_command(temp_buffer);

            command_buffer_pos = 0;
            cursor_pos = 0;
            command_buffer[0] = U'\0';

            console_puts(SHELL_PROMPT);
            return;
        }

        if (c == U'\t') {
            size_t spaces_to_add = 4;
            if (command_buffer_pos + spaces_to_add < MAX_COMMAND_LENGTH) {
                memmove(&command_buffer[cursor_pos + spaces_to_add], &command_buffer[cursor_pos], 
                        (command_buffer_pos - cursor_pos) * sizeof(uint32_t));
                
                for (size_t i = 0; i < spaces_to_add; i++) command_buffer[cursor_pos + i] = U' ';
                
                command_buffer_pos += spaces_to_add;
                command_buffer[command_buffer_pos] = U'\0';
                
                console_puts(&command_buffer[cursor_pos]);
                cursor_pos += spaces_to_add;

                for (size_t j = 0; j < (command_buffer_pos - cursor_pos); j++) {
                    console_move_cursor_left(false);
                }
            }
            return;
        }

        if (command_buffer_pos < MAX_COMMAND_LENGTH - 1) {
            memmove(&command_buffer[cursor_pos + 1], &command_buffer[cursor_pos], (command_buffer_pos - cursor_pos) * sizeof(uint32_t));
            
            command_buffer[cursor_pos] = c;
            command_buffer_pos++;
            command_buffer[command_buffer_pos] = U'\0';
            
            console_putc(c);
            cursor_pos++;

            if (cursor_pos < command_buffer_pos) {
                console_puts(&command_buffer[cursor_pos]);
                for (size_t i = 0; i < (command_buffer_pos - cursor_pos); i++) {
                    console_move_cursor_left(false);
                }
            }
            return;
        }
    }   
}

void shell_register_command(const shell_command_t *command) {
    if (!command) return;

    serial_printf("Shell: Registering command: %s\n", command->name);
    
    if (commands.command_count >= MAX_COMMANDS) {
        serial_printf("Shell: Too many commands registered!\n");
        return;
    }

    commands.commands[commands.command_count].name = command->name;
    commands.commands[commands.command_count].handler = command->handler;
    commands.commands[commands.command_count].description = command->description;
    commands.command_count++;
}