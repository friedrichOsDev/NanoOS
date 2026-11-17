/**
 * @file shell.c
 * @brief Shell subsystem initialization.
 * @author friedrichOsDev
 */

#include "_shell.h"
#include <shell/shell.h>

bool shell_initialized = false;

/**
 * @brief Initializes shell input handling and the input renderer.
 *
 * @return E_OK on success, or an error code.
 */
kerr_t shell_init(void) {
    K_TRY(shell_input_init());
    K_TRY(shell_init_renderer());

    shell_initialized = true;

    log_info("shell renderer initialized");
    return E_OK;
}
