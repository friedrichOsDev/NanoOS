/**
 * @file input_dispatch.c
 * @brief Central input event dispatch and handler registration.
 * @author friedrichOsDev
 */

#include <drivers/hid/input.h>
#include <kernel/sync.h>
#include <sched/process.h>
#include <sched/thread.h>

#define INPUT_MAX_HANDLERS 16

typedef struct {
    input_event_handler_t callback;
    void *context;
} input_handler_entry_t;

static input_handler_entry_t handlers[INPUT_MAX_HANDLERS];
static size_t handler_count;
static spinlock_t handler_lock = SPINLOCK_INIT;
static bool dispatcher_started;

/**
 * @brief Register a callback for translated input events.
 *
 * Handlers must remain valid for the lifetime of the kernel. Callbacks are
 * invoked by the input dispatcher thread and never while the registry lock
 * is held.
 *
 * @param[in] handler Event callback.
 * @param[in] context Opaque callback context.
 * @return E_OK on success, E_INVAL for an invalid callback, or E_NOMEM
 *         when the handler registry is full.
 */
kerr_t input_register_handler(input_event_handler_t handler, void *context) {
    if (!handler) {
        return E_INVAL;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&handler_lock);
    for (size_t i = 0; i < handler_count; i++) {
        if (handlers[i].callback == handler && handlers[i].context == context) {
            spinlock_release_irqrestore(&handler_lock, flags);
            return E_INVAL;
        }
    }

    if (handler_count >= INPUT_MAX_HANDLERS) {
        spinlock_release_irqrestore(&handler_lock, flags);
        return E_NOMEM;
    }

    handlers[handler_count].callback = handler;
    handlers[handler_count].context = context;
    handler_count++;
    spinlock_release_irqrestore(&handler_lock, flags);
    return E_OK;
}

/**
 * @brief Pull input events and distribute them to registered kernel handlers.
 *
 * @param[in] arg Unused thread argument.
 */
static void input_dispatcher_thread(void *arg) {
    (void)arg;

    for (;;) {
        input_event_t event;
        kerr_t err = input_poll_event(&event);
        if (err == E_NOTAVAIL) {
            thread_yield();
            continue;
        }
        if (k_failed(err)) {
            log_warn("input dispatcher polling failed (%s)", kerr_str(err));
            thread_yield();
            continue;
        }

        input_handler_entry_t snapshot[INPUT_MAX_HANDLERS];
        size_t snapshot_count;

        irq_flags_t flags = spinlock_acquire_irqsave(&handler_lock);
        snapshot_count = handler_count;
        for (size_t i = 0; i < snapshot_count; i++) {
            snapshot[i] = handlers[i];
        }
        spinlock_release_irqrestore(&handler_lock, flags);

        for (size_t i = 0; i < snapshot_count; i++) {
            snapshot[i].callback(&event, snapshot[i].context);
        }
    }
}

/**
 * @brief Start the central input dispatcher thread.
 *
 * @return E_OK on success, E_INVAL if input is not initialized, or an
 *         error returned by the thread subsystem.
 */
kerr_t input_dispatcher_start() {
    static thread_t *dispatcher_thread;

    irq_flags_t flags = spinlock_acquire_irqsave(&handler_lock);
    if (dispatcher_started) {
        spinlock_release_irqrestore(&handler_lock, flags);
        return E_OK;
    }
    dispatcher_started = true;
    spinlock_release_irqrestore(&handler_lock, flags);

    kerr_t err = thread_create(proc_get_kernel_proc(), input_dispatcher_thread,
                               NULL, "input_dispatcher", &dispatcher_thread);
    if (k_failed(err)) {
        flags = spinlock_acquire_irqsave(&handler_lock);
        dispatcher_started = false;
        spinlock_release_irqrestore(&handler_lock, flags);
        return err;
    }

    return E_OK;
}
