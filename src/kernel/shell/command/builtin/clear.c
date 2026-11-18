/**
 * @file clear.c
 * @brief Built-in clear command.
 * @author friedrichOsDev
 */

#include "../_command.h"

#include <shell/output.h>

/** @brief Clear the shell output buffer. */
kerr_t shell_cmd_clear(int argc, const char *const argv[]) {
    (void)argv;

    if (argc != 1) {
        (void)shell_write_line("Usage: clear");
        return E_INVAL;
    }

    return shell_clear_output();
}
