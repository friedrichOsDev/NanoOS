/**
 * @file shell.c
 * @brief Shell subsystem initialization and lifecycle management.
 * @author friedrichOsDev
 */

#include "_shell.h"
#include <shell/shell.h>

bool shell_initialized = false;

/**
 * @brief Initializes shell input handling and the asynchronous input renderer.
 *
 * @return E_OK on success, or an error code on failure.
 */
kerr_t shell_init() {
    if (__atomic_load_n(&shell_initialized, __ATOMIC_ACQUIRE)) {
        return E_OK;
    }

    K_TRY(shell_input_init());

    kerr_t err = shell_init_renderer();
    if (k_failed(err)) {
        (void)shell_input_deinit();
        return err;
    }

    __atomic_store_n(&shell_initialized, true, __ATOMIC_RELEASE);
    log_info("shell initialized");
    return E_OK;
}

/**
 * @brief Requests shell shutdown without waiting for the renderer thread.
 *
 * The first call disables input and requests renderer termination. It returns
 * E_NOTAVAIL while the renderer is still releasing its resources. Call again
 * after that to complete the lifecycle transition and receive E_OK.
 *
 * @return E_OK when shutdown is complete, E_NOTAVAIL while it is pending.
 */
kerr_t shell_deinit() {
    __atomic_store_n(&shell_initialized, false, __ATOMIC_RELEASE);

    (void)shell_input_deinit();

    return shell_deinit_renderer();
}
