/**
 * @file mouse.c
 * @brief PS/2 mouse packet parser (3-byte and IntelliMouse 4-byte).
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <drivers/hid/ps2/ps2.h>

static uint8_t packet[4];
static uint8_t packet_index;
static uint8_t packet_size = 3;
static uint8_t buttons;
static volatile uint64_t mouse_packet_count;
static volatile uint64_t mouse_sync_discard_count;
static volatile uint64_t mouse_move_event_count;
static volatile uint64_t mouse_zero_delta_count;

/**
 * @brief Set the mouse packet size based on the detected device identification byte.
 *
 * @param[in] id The ID to set.
 */
void ps2_mouse_set_id(uint8_t id) {
    packet_size = (id == 0x03 || id == 0x04) ? 4 : 3;
    packet_index = 0;
}

/** @brief Reset the internal PS/2 mouse packet parser state, indices, and button tracking. */
void ps2_mouse_reset_parser() {
    packet_index = 0;
    packet_size = 3;
    buttons = 0;
    mouse_packet_count = 0U;
    mouse_sync_discard_count = 0U;
    mouse_move_event_count = 0U;
    mouse_zero_delta_count = 0U;
}

/**
 * @brief Log cumulative mouse packet synchronization and movement diagnostics.
 */
void ps2_mouse_log_diagnostics() {
    log_trace("PS/2 mouse stats: packet_size=%u packets=%llu sync_discards=%llu move_events=%llu zero_delta=%llu",
              (unsigned)packet_size, mouse_packet_count,
              mouse_sync_discard_count, mouse_move_event_count,
              mouse_zero_delta_count);
}

/**
 * @brief Process and decode an incoming raw byte from the PS/2 mouse hardware data stream.
 *
 * @param[in] byte The byte to parse.
 */
void ps2_mouse_byte(uint8_t byte) {
    if (packet_index == 0 && !(byte & 0x08)) {
        mouse_sync_discard_count++;
        return;
    }
    packet[packet_index++] = byte;
    if (packet_index < packet_size)
        return;
    packet_index = 0;
    mouse_packet_count++;

    /*
     * Bytes 1 and 2 are already signed 8-bit two's-complement values.
     * Casting through int8_t performs the sign extension. The sign bits in
     * byte 0 are consistency flags only; subtracting 256 again would turn
     * every negative delta into an erroneous value around -256 (for example,
     * -1 would become -257).
     *
     * On overflow the corresponding movement counter is invalid. Discard
     * only that axis while still processing the other axis and button state.
     */
    int32_t dx = (int32_t)(int8_t)packet[1];
    int32_t dy = (int32_t)(int8_t)packet[2];

    if (packet[0] & 0x40)
        dx = 0;
    if (packet[0] & 0x80)
        dy = 0;

    if (dx != 0 || dy != 0) {
        hid_event_t event = {.type = HID_EVENT_MOUSE_MOVE, .timestamp = hpet_get_timestamp()};
        event.mouse_move.dx = dx;
        event.mouse_move.dy = -(int64_t)dy;
        if (k_succeeded(hid_push_event(&event))) {
            mouse_move_event_count++;
        }
    } else {
        mouse_zero_delta_count++;
    }

    uint8_t next = packet[0] & 0x07;
    const uint8_t masks[] = {MOUSE_BTN_LEFT, MOUSE_BTN_RIGHT, MOUSE_BTN_MIDDLE};
    for (uint8_t i = 0; i < 3; i++) {
        if ((next & (1U << i)) != (buttons & (1U << i))) {
            hid_event_t event = {
                .type = (next & (1U << i)) ? HID_EVENT_MOUSE_BUTTON_PRESS : HID_EVENT_MOUSE_BUTTON_RELEASE,
                .timestamp = hpet_get_timestamp()};
            event.mouse_btn.button = masks[i];
            (void)hid_push_event(&event);
        }
    }
    buttons = next;

    if (packet_size == 4) {
        int8_t wheel = (int8_t)(packet[3] << 4) >> 4;
        if (wheel) {
            hid_event_t event = {.type = HID_EVENT_MOUSE_SCROLL, .timestamp = hpet_get_timestamp()};
            event.mouse_scroll.delta = wheel;
            (void)hid_push_event(&event);
        }
    }
}
