/**
 * @file shell_input.c
 * @brief Kernel shell input subsystem.
 * @author friedrichOsDev
 */

#include "shell_input.h"
#include <drivers/hid/input.h>
#include <pch.h>
#include <sched/sched.h>
#include <shell/shell.h>

static void shell_handle_event(input_event_t *event) {
    // handle event
    // TODO
}

/**
 * @brief Pulls input events from the input subsystem and calls the shell event handler.
 *
 * @param[in] arg Unused argument.
 */
void shell_input_thread(void *arg) {
    (void)arg;

    while (!shell_is_running()) {
        // thread started before initialization is finished
        thread_yield();
    }

    while (shell_is_running()) {
        input_event_t event;
        kerr_t err = input_poll_event(&event);
        if (err == E_NOTAVAIL) {
            thread_yield();
            continue;
        }
        if (k_failed(err)) {
            log_warn("input event polling failed (%s)", kerr_str(err));
            thread_yield();
            continue;
        }
        shell_handle_event(&event);
    }

    // shell has exited
    thread_exit();
}
