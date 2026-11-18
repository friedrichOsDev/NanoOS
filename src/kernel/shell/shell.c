/**
 * @file shell.c
 * @brief Shell subsystem initialization and lifecycle management.
 * @author friedrichOsDev
 */
#include "_shell.h"
#include "command/_command.h"
#include <shell/shell.h>
#include <shell/output.h>

bool shell_initialized = false;

/**
 * @brief Initialize output, command execution, renderer, and finally input.
 *
 * The input callback is registered last. shell_initialized is published
 * immediately before registration, so the dispatcher can never observe the
 * callback while the shell still rejects all events as uninitialized.
 */
kerr_t shell_init() {
    if (__atomic_load_n(&shell_initialized, __ATOMIC_ACQUIRE)) {
        return E_OK;
    }

    /*
     * Shutdown is asynchronous. Do not reset shared state while a previous
     * renderer/worker is still exiting. Once both have stopped, finalize their
     * state flags before reinitializing them.
     */
    if (!shell_command_is_stopped() || !shell_renderer_is_stopped()) {
        return E_NOTAVAIL;
    }

    kerr_t err = shell_command_deinit();
    if (k_failed(err)) {
        return err;
    }

    err = shell_deinit_renderer();
    if (k_failed(err)) {
        return err;
    }

    err = shell_output_init();
    if (k_failed(err)) {
        return err;
    }

    err = shell_command_init();
    if (k_failed(err)) {
        (void)shell_output_deinit();
        return err;
    }

    err = shell_init_renderer();
    if (k_failed(err)) {
        /*
         * The command worker may already be scheduled. Stop it cooperatively
         * and keep output alive until its drain/exit has completed.
         */
        if (shell_command_deinit() == E_OK) {
            (void)shell_output_deinit();
        }
        return err;
    }

    /*
     * The renderer and command worker now have all their dependencies.
     * Publish initialized before making the callback visible to the dispatcher.
     */
    __atomic_store_n(&shell_initialized, true, __ATOMIC_RELEASE);

    err = shell_input_init();
    if (k_failed(err)) {
        __atomic_store_n(&shell_initialized, false, __ATOMIC_RELEASE);

        /*
         * Registration failed, so no shell callback was installed. Request
         * orderly shutdown; output remains available while worker/renderer exit.
         */
        (void)shell_command_deinit();
        (void)shell_deinit_renderer();
        return err;
    }

    return E_OK;
}

/**
 * @brief Request shell shutdown; retry while command worker/renderer finish.
 *
 * Input is unregistered first, accepted requests are drained, then renderer
 * and output are stopped in that order.
 *
 * @return E_OK when shutdown is complete, or E_NOTAVAIL while pending.
 */
kerr_t shell_deinit() {
    __atomic_store_n(&shell_initialized, false, __ATOMIC_RELEASE);

    kerr_t err = shell_input_deinit();
    if (err == E_NOTAVAIL) {
        return E_NOTAVAIL;
    }
    if (k_failed(err)) {
        return err;
    }

    err = shell_command_deinit();
    if (err == E_NOTAVAIL) {
        return E_NOTAVAIL;
    }
    if (k_failed(err)) {
        return err;
    }

    err = shell_deinit_renderer();
    if (err == E_NOTAVAIL) {
        return E_NOTAVAIL;
    }
    if (k_failed(err)) {
        return err;
    }

    return shell_output_deinit();
}
