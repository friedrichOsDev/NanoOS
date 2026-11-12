/**
 * @file hid.c
 * @brief Human interface driver HAL
 * @author friedrichOsDev
 */

#include <drivers/hid/hid.h>
#include <lib/types/ringbuffer.h>
#include <mm/memory.h>
#include <pch.h>

#define HID_EVENT_QUEUE_CAPACITY (256 * sizeof(hid_event_t))

static hid_driver_t *driver_list = NULL;
static ringbuffer_t *event_queue = NULL;
static bool hid_initialized = false;
static volatile uint64_t hid_events_enqueued;
static volatile uint64_t hid_events_dropped;
static volatile uint64_t hid_events_dequeued;

/**
 * @brief Initializes the HID subsystem and event ring buffer.
 *
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t hid_init() {
    if (hid_initialized) {
        return E_OK;
    }

    K_TRY(ringbuffer_create(HID_EVENT_QUEUE_CAPACITY, &event_queue));

    hid_initialized = true;
    log_debug("hid initialized successfully");
    return E_OK;
}

/**
 * @brief Deinitializes the HID subsystem, stops drivers, and frees resources.
 *
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t hid_deinit() {
    if (!hid_initialized) {
        return E_OK;
    }

    hid_driver_t *current = driver_list;
    while (current != NULL) {
        if (current->disable) {
            current->disable();
        }
        if (current->deinit) {
            current->deinit();
        }
        current = current->next;
    }

    if (event_queue) {
        ringbuffer_destroy(event_queue);
        event_queue = NULL;
    }

    hid_initialized = false;
    log_debug("hid subsystem deinitialized");
    return E_OK;
}

/**
 * @brief Registers, initializes, and enables a new HID driver.
 *
 * @param[in] driver Pointer to the HID driver structure.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t hid_register_driver(hid_driver_t *driver) {
    if (!driver) {
        return E_INVAL;
    }

    if (driver->init) {
        K_TRY(driver->init());
    }

    if (driver->enable) {
        driver->enable();
    }

    driver->next = driver_list;
    driver_list = driver;

    log_debug("hid registered and enabled driver: %s", driver->driver_name ? driver->driver_name : "unnamed");
    return E_OK;
}

/**
 * @brief Pushes a new input event into the global HID event queue.
 *
 * Can be safely called from hardware interrupt handlers (e.g., PS/2 IRQs).
 *
 * @param[in] event Pointer to the HID event structure.
 * @return E_OK on success, or E_NOMEM if the event queue is full.
 */
kerr_t hid_push_event(const hid_event_t *event) {
    if (!event || !event_queue || !hid_initialized) {
        return E_INVAL;
    }

    size_t written = ringbuffer_write(event_queue, event, sizeof(hid_event_t));
    if (written != sizeof(hid_event_t)) {
        hid_events_dropped++;
        log_warn("hid event queue full, dropping event type %d", event->type);
        return E_NOMEM;
    }

    hid_events_enqueued++;
    return E_OK;
}

/**
 * @brief Polls/reads the next available input event from the HID event queue.
 *
 * @param[out] out_event Pointer to store the retrieved event.
 * @return E_OK on success, or E_NOTAVAIL if no events are pending.
 */
kerr_t hid_poll_event(hid_event_t *out_event) {
    if (!out_event || !event_queue || !hid_initialized) {
        return E_INVAL;
    }

    size_t read_bytes = ringbuffer_read(event_queue, out_event, sizeof(hid_event_t));
    if (read_bytes != sizeof(hid_event_t)) {
        return E_NOTAVAIL;
    }

    hid_events_dequeued++;
    return E_OK;
}

/**
 * @brief Log HID event queue counters and current backlog outside interrupt context.
 */
void hid_log_diagnostics() {
    size_t pending = event_queue ? ringbuffer_available(event_queue) /
                                       sizeof(hid_event_t)
                                 : 0U;
    log_trace("HID queue stats: enqueued=%llu dequeued=%llu dropped=%llu pending=%u",
              hid_events_enqueued, hid_events_dequeued, hid_events_dropped,
              (unsigned)pending);
}
