/**
 * @file echo.c
 * @brief Built-in echo command.
 * @author friedrichOsDev
 */

#include "../_command.h"

#include <shell/output.h>

/** @brief Print all arguments after argv[0], separated by one space. */
kerr_t shell_cmd_echo(int argc, const char *const argv[]) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) {
            K_TRY(shell_write(" "));
        }
        K_TRY(shell_write(argv[i]));
    }

    return shell_write_u32((uint32_t)'\n');
}
