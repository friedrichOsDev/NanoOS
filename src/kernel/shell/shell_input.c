/**
 * @file shell_input.c
 * @brief Kernel shell keyboard input worker.
 * @author friedrichOsDev
 */

#include "shell_input.h"
#include "shell_internal.h"
#include <drivers/hid/input.h>
#include <sched/sched.h>
#include <shell/shell.h>

/**
 * @brief Polls translated HID events and forwards keyboard events to the shell.
 * @param[in] arg Unused thread argument.
 */
void shell_input_thread(void *arg) {
    (void)arg;
    while (shell_is_running()) {
        input_event_t event;
        kerr_t err = input_poll_event(&event);
        if (err == E_NOTAVAIL) {
            thread_sleep_ms(10U);
            continue;
        }
        if (k_failed(err)) {
            log_warn("shell input polling failed (%s)", kerr_str(err));
            thread_yield();
            continue;
        }
        shell_handle_event(&event);
    }
    thread_exit();
}
