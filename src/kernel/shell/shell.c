/**
 * @file shell.c
 * @brief Kernel shell state, terminal output and command dispatch.
 * @author friedrichOsDev
 */

#include <drivers/gfx/compositor.h>
#include "shell_internal.h"
#include <kernel/sync.h>
#include <mm/memory.h>
#include <shell/shell.h>
#include <sched/process.h>
#include <sched/thread.h>
#include "shell_renderer.h"

// #define SHELL_PROMPT "friedrichOsDev@nanoos> "

static volatile bool shell_running = false;
static bool shell_initialized = false;
// static spinlock_t shell_lock = SPINLOCK_INIT;
static vmm_space_t shell_space;
static process_t *shell_process = NULL;
static layer_t *shell_layer = NULL;

static shell_screen_t shell_screen = {0};


/**
 * @brief Initializes the terminal, its process and its input/render threads.
 * @return E_OK on success, or an error code.
 */
kerr_t shell_init(void) {
    if (shell_initialized) return E_NOTAVAIL;

    rect_size_t screen = compositor_get_size();
    if (screen.width == 0U || screen.height < 48U) {
        return E_NOINIT;
    }


    for (size_t row = 0U; row < SHELL_OUTPUT_LINES; row++) {
        shell_screen.rows[row].chars[0].c = 'a';
    }
    shell_screen.dirty = true;

    K_TRY(vmm_space_create(&shell_space));
    kerr_t err = proc_create("shell", &shell_space, &shell_process);
    if (k_failed(err)) {
        vmm_space_destroy(&shell_space);
        return err;
    }

    err = layer_create(screen, 1000, &shell_layer);
    if (k_failed(err)) {
        proc_destroy(shell_process);
        shell_process = NULL;
        vmm_space_destroy(&shell_space);
        return err;
    }

    shell_running = true;
    thread_t *render_thread = NULL;
    err = thread_create(shell_process, shell_render_thread, NULL, "shell renderer", &render_thread);
    if (k_failed(err)) {
        shell_running = false;
        layer_destroy(shell_layer);
        shell_layer = NULL;
        proc_destroy(shell_process);
        shell_process = NULL;
        vmm_space_destroy(&shell_space);
        return err;
    }

    // thread_t *command_thread = NULL;
    // err = thread_create(shell_process, shell_command_thread, NULL, "shell command worker", &command_thread);
    // if (k_failed(err)) {
    //     shell_running = false;
    //     layer_destroy(shell_layer);
    //     shell_layer = NULL;
    //     proc_unregister(shell_process);
    //     kfree((virt_addr_t)shell_process);
    //     shell_process = NULL;
    //     vmm_space_destroy(&shell_space);
    //     return err;
    // }

    // thread_t *input_thread = NULL;
    // err = thread_create(shell_process, shell_input_thread, NULL, "shell input", &input_thread);
    // if (k_failed(err)) {
    //     shell_running = false;
    //     return err;
    // }

    shell_initialized = true;
    return E_OK;
}

shell_screen_t *shell_get_screen() {
    return &shell_screen;
}

layer_t *shell_get_layer() {
    return shell_layer;
}

bool shell_is_running() {
    return shell_running;
}
