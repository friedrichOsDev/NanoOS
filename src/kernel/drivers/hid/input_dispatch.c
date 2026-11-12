/**
 * @file input_dispatch.c
 * @brief Central input event dispatch and handler registration.
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <drivers/hid/input.h>
#include <drivers/hid/ps2/ps2.h>
#include <kernel/sync.h>
#include <sched/process.h>
#include <sched/thread.h>
#include <sched/sched.h>

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
 * @param[in] handler Event callback.
 * @param[in] context Opaque callback context.
 * @return E_OK on success, E_INVAL for an invalid callback, or E_NOMEM
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
    size_t registered_count = handler_count;
    spinlock_release_irqrestore(&handler_lock, flags);
    log_trace("input handler registered: callback=%p context=%p count=%u",
              handler, context, (unsigned)registered_count);
    return E_OK;
}

/**
 * @brief Pull input events and distribute them to registered kernel handlers.
 * 
 * @param[in] arg Unused thread argument.
 */
static void input_dispatcher_thread(void *arg) {
    (void)arg;

    uint64_t dispatched_events = 0U;
    uint64_t idle_polls = 0U;
    uint64_t last_report_ms = hpet_get_timestamp();
    log_info("input dispatcher thread online");

    for (;;) {
        input_event_t event;
        kerr_t err = input_poll_event(&event);
        if (err == E_NOTAVAIL) {
            idle_polls++;
            thread_yield();
        } else if (k_failed(err)) {
            log_warn("input dispatcher polling failed (%s)", kerr_str(err));
            thread_yield();
        } else {
            dispatched_events++;
            input_handler_entry_t snapshot[INPUT_MAX_HANDLERS];
            size_t snapshot_count;

            irq_flags_t flags = spinlock_acquire_irqsave(&handler_lock);
            snapshot_count = handler_count;
            for (size_t i = 0; i < snapshot_count; i++) {
                snapshot[i] = handlers[i];
            }
            spinlock_release_irqrestore(&handler_lock, flags);

            if (event.hid.type == HID_EVENT_KEY_PRESS ||
                event.hid.type == HID_EVENT_KEY_RELEASE) {
                log_trace("input key event: type=%u key=%u scan=%02x action=%u unicode=%u handlers=%u",
                          (unsigned)event.hid.type,
                          (unsigned)event.hid.key.keycode,
                          event.hid.key.scancode,
                          (unsigned)event.key_action,
                          event.unicode_count ? event.unicode : 0U,
                          (unsigned)snapshot_count);
            } else if (event.hid.type != HID_EVENT_MOUSE_MOVE) {
                log_trace("input pointer event: type=%u handlers=%u",
                          (unsigned)event.hid.type, (unsigned)snapshot_count);
            }

            for (size_t i = 0; i < snapshot_count; i++) {
                if (event.hid.type != HID_EVENT_MOUSE_MOVE) {
                    log_trace("input dispatch callback %u/%u begin",
                              (unsigned)(i + 1U), (unsigned)snapshot_count);
                }
                snapshot[i].callback(&event, snapshot[i].context);
                if (event.hid.type != HID_EVENT_MOUSE_MOVE) {
                    log_trace("input dispatch callback %u/%u end",
                              (unsigned)(i + 1U), (unsigned)snapshot_count);
                }
            }
        }

        uint64_t now_ms = hpet_get_timestamp();
        if (now_ms - last_report_ms >= 1000U) {
            log_trace("input dispatcher alive: dispatched=%llu idle_polls=%llu handlers=%u",
                      dispatched_events, idle_polls, (unsigned)handler_count);
            hid_log_diagnostics();
            ps2_log_diagnostics();
            ps2_mouse_log_diagnostics();
            input_log_diagnostics();
            last_report_ms = now_ms;
        }
    }
}

/**
 * @brief Start the central input dispatcher thread.
 * 
 * @return E_OK on success, E_INVAL if input is not initialized, or an
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

    log_info("input dispatcher thread created");
    return E_OK;
}
