/**
 * @file shell_renderer.c
 * @brief Kernel shell rendering subsystem.
 * @author friedrichOsDev
 */

#include "shell_renderer.h"
#include <sched/sched.h>
#include <shell/shell.h>

void shell_render_thread(void *arg) {
    (void)arg;

    while (!shell_is_running()) {
        // thread started before initialization is finished
        thread_yield();
    }

    while (shell_is_running()) {
        // TODO
    }

    // shell has exited
    thread_exit();
}
