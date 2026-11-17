/**
 * @file shell.c
 * @brief Shell subsystem initialization and lifecycle management.
 * @author friedrichOsDev
 */

#include "_shell.h"
#include <shell/shell.h>\n#include <shell/output.h>

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

    K_TRY(shell_output_init());\n\n    kerr_t input_err = shell_input_init();\n    if (k_failed(input_err)) {\n        (void)shell_output_deinit();\n        return input_err;\n    }

    kerr_t err = shell_init_renderer();
    if (k_failed(err)) {
        (void)shell_input_deinit();
        return err;
    }

    __atomic_store_n(&shell_initialized, true, __ATOMIC_RELEASE);
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

    kerr_t err = shell_deinit_renderer();\n    if (k_failed(err)) {\n        return err;\n    }\n\n    return shell_output_deinit();
}
