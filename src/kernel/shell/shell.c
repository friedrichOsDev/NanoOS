/**
 * @file shell.c
 * @brief Kernel shell / terminal interface.
 * @author friedrichOsDev
 */

#include "shell_input.h"
#include "shell_renderer.h"
#include <sched/process.h>
#include <sched/thread.h>
#include <shell/shell.h>

static bool shell_running = false;

kerr_t shell_init() {
    vmm_space_t shell_space;
    K_TRY(vmm_space_create(&shell_space));
    process_t *shell_proc;
    K_TRY(proc_create("shell", &shell_space, &shell_proc));

    thread_t *shell_ipt_thread;
    K_TRY(thread_create(shell_proc, shell_input_thread, nullptr, "shell input thread", &shell_ipt_thread));

    thread_t *shell_ui_thread;
    K_TRY(thread_create(shell_proc, shell_render_thread, nullptr, "shell render thread", &shell_ui_thread));

    shell_running = true;

    return E_OK;
}

bool shell_is_running() {
    return shell_running;
}

void shell_deinit() {
    shell_running = false;

    // deinit
}
