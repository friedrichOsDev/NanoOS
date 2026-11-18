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

/** @brief Initialize output, input, command execution and renderer. */
kerr_t shell_init() {
    if (__atomic_load_n(&shell_initialized, __ATOMIC_ACQUIRE)) return E_OK;

    /* Never reset shared state while a previous renderer/worker is still exiting. */
    if (!shell_command_is_stopped() || !shell_renderer_is_stopped()) return E_NOTAVAIL;

    K_TRY(shell_output_init());
    kerr_t err = shell_input_init();
    if (k_failed(err)) {
        (void)shell_output_deinit();
        return err;
    }

    err = shell_command_init();
    if (k_failed(err)) {
        (void)shell_input_deinit();
        (void)shell_output_deinit();
        return err;
    }

    err = shell_init_renderer();
    if (k_failed(err)) {
        (void)shell_input_deinit();
        /*
         * The worker may still need the output buffer while draining. Keep
         * output initialized until shell_deinit() observes the worker stopped.
         */
        if (shell_command_deinit() == E_OK) (void)shell_output_deinit();
        return err;
    }

    __atomic_store_n(&shell_initialized, true, __ATOMIC_RELEASE);
    return E_OK;
}

/**
 * @brief Request shell shutdown; retry while command worker/renderer finish.
 * Input is disabled first, accepted requests are drained, then renderer and
 * output are stopped in that order.
 */
kerr_t shell_deinit() {
    __atomic_store_n(&shell_initialized, false, __ATOMIC_RELEASE);

    kerr_t err = shell_input_deinit();
    if (err == E_NOTAVAIL) return E_NOTAVAIL;
    if (k_failed(err)) return err;

    err = shell_command_deinit();
    if (err == E_NOTAVAIL) return E_NOTAVAIL;
    if (k_failed(err)) return err;

    err = shell_deinit_renderer();
    if (err == E_NOTAVAIL) return E_NOTAVAIL;
    if (k_failed(err)) return err;

    return shell_output_deinit();
}
