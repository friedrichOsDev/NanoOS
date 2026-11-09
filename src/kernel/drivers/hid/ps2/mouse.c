/**
 * @file mouse.c
 * @brief PS/2 mouse packet parser (3-byte and IntelliMouse 4-byte).
 */
#include <drivers/hid/ps2/ps2.h>
#include <arch/arch.h>

static uint8_t packet[4];
static uint8_t packet_index;
static uint8_t packet_size = 3;
static uint8_t buttons;

void ps2_mouse_set_id(uint8_t id) {
    packet_size = (id == 0x03 || id == 0x04) ? 4 : 3;
    packet_index = 0;
}

void ps2_mouse_reset_parser(void) {
    packet_index = 0;
    packet_size = 3;
    buttons = 0;
}

void ps2_mouse_byte(uint8_t byte) {
    if (packet_index == 0 && !(byte & 0x08)) return;
    packet[packet_index++] = byte;
    if (packet_index < packet_size) return;
    packet_index = 0;

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

    if (packet[0] & 0x40) dx = 0;
    if (packet[0] & 0x80) dy = 0;

    if (dx != 0 || dy != 0) {
        hid_event_t event = {.type = HID_EVENT_MOUSE_MOVE, .timestamp = hpet_get_timestamp()};
        event.mouse_move.dx = dx;
        event.mouse_move.dy = -(int64_t)dy;
        (void)hid_push_event(&event);
    }

    uint8_t next = packet[0] & 0x07;
    const uint8_t masks[] = {MOUSE_BTN_LEFT, MOUSE_BTN_RIGHT, MOUSE_BTN_MIDDLE};
    for (uint8_t i = 0; i < 3; i++) {
        if ((next & (1U << i)) != (buttons & (1U << i))) {
            hid_event_t event = {
                .type = (next & (1U << i)) ? HID_EVENT_MOUSE_BUTTON_PRESS : HID_EVENT_MOUSE_BUTTON_RELEASE,
                .timestamp = hpet_get_timestamp()
            };
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
