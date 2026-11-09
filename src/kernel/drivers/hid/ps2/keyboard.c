/**
 * @file keyboard.c
 * @brief PS/2 Set-2 keyboard scan-code parser.
 */
#include <drivers/hid/ps2/ps2.h>
#include <arch/arch.h>

static bool extended;
static bool releasing;
static bool pause_sequence;
static uint8_t pause_index;

static hid_keycode_t decode(uint8_t code, bool ext) {
    if (ext) {
        switch (code) {
        case 0x14: return KEY_RCONTROL;
        case 0x11: return KEY_RMENU;
        case 0x5A: return KEY_NUMLOCK_RETURN;
        case 0x4A: return KEY_NUMLOCK_DIVIDE;
        case 0x70: return KEY_INSERT;
        case 0x71: return KEY_DELETE;
        case 0x6C: return KEY_HOME;
        case 0x69: return KEY_END;
        case 0x7D: return KEY_PRIOR;
        case 0x7A: return KEY_NEXT;
        case 0x75: return KEY_UP;
        case 0x6B: return KEY_LEFT;
        case 0x72: return KEY_DOWN;
        case 0x74: return KEY_RIGHT;
        default: return KEY_UNKNOWN;
        }
    }
    switch (code) {
    case 0x76: return KEY_ESC;
    case 0x05: return KEY_F1; case 0x06: return KEY_F2; case 0x04: return KEY_F3;
    case 0x0C: return KEY_F4; case 0x03: return KEY_F5; case 0x0B: return KEY_F6;
    case 0x83: return KEY_F7; case 0x0A: return KEY_F8; case 0x01: return KEY_F9;
    case 0x09: return KEY_F10; case 0x78: return KEY_F11; case 0x07: return KEY_F12;
    case 0x16: return KEY_1; case 0x1E: return KEY_2; case 0x26: return KEY_3;
    case 0x25: return KEY_4; case 0x2E: return KEY_5; case 0x36: return KEY_6;
    case 0x3D: return KEY_7; case 0x3E: return KEY_8; case 0x46: return KEY_9;
    case 0x45: return KEY_0; case 0x66: return KEY_BACK; case 0x0D: return KEY_TAB;
    case 0x15: return KEY_Q; case 0x1D: return KEY_W; case 0x24: return KEY_E;
    case 0x2D: return KEY_R; case 0x2C: return KEY_T; case 0x35: return KEY_Z;
    case 0x3C: return KEY_U; case 0x43: return KEY_I; case 0x44: return KEY_O;
    case 0x4D: return KEY_P; case 0x5A: return KEY_RETURN; case 0x58: return KEY_CAPITAL;
    case 0x1C: return KEY_A; case 0x1B: return KEY_S; case 0x23: return KEY_D;
    case 0x2B: return KEY_F; case 0x34: return KEY_G; case 0x33: return KEY_H;
    case 0x3B: return KEY_J; case 0x42: return KEY_K; case 0x4B: return KEY_L;
    case 0x12: return KEY_LSHIFT; case 0x59: return KEY_RSHIFT;
    case 0x1A: return KEY_Y; case 0x22: return KEY_X; case 0x21: return KEY_C;
    case 0x2A: return KEY_V; case 0x32: return KEY_B; case 0x31: return KEY_N;
    case 0x3A: return KEY_M; case 0x14: return KEY_LCONTROL;
    case 0x11: return KEY_LMENU; case 0x29: return KEY_SPACE;
    case 0x77: return KEY_NUMLOCK; case 0x7E: return KEY_SCROLL;
    case 0x7C: return KEY_NUMLOCK_MULTIPLY; case 0x7B: return KEY_NUMLOCK_SUBTRACT;
    case 0x79: return KEY_NUMLOCK_ADD;
    default: return KEY_UNKNOWN;
    }
}

void ps2_keyboard_reset_parser(void) {
    extended = false; releasing = false; pause_sequence = false; pause_index = 0;
}

void ps2_keyboard_byte(uint8_t byte) {
    static const uint8_t pause_tail[] = {0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77};
    if (pause_sequence) {
        if (pause_index < sizeof(pause_tail) && byte == pause_tail[pause_index]) {
            pause_index++;
            if (pause_index == sizeof(pause_tail)) {
                hid_event_t event = {.type = HID_EVENT_KEY_PRESS, .timestamp = hpet_get_timestamp()};
                event.key.keycode = KEY_PAUSE;
                event.key.scancode = 0xE1;
                (void)hid_push_event(&event);
                event.type = HID_EVENT_KEY_RELEASE;
                (void)hid_push_event(&event);
                pause_sequence = false;
            }
        } else {
            pause_sequence = false;
            pause_index = 0;
        }
        return;
    }
    if (byte == 0xFA || byte == 0xFE || byte == 0xAA) return;
    if (byte == 0xE1) { pause_sequence = true; pause_index = 0; return; }
    if (byte == 0xE0) { extended = true; return; }
    if (byte == 0xF0) { releasing = true; return; }

    hid_keycode_t key = decode(byte, extended);
    /*
     * Preserve unmapped scan codes as KEY_UNKNOWN events. The debug HID
     * monitor can then reveal missing mappings without logging from IRQ
     * context or losing the raw byte.
     */
    hid_event_t event = {
        .type = releasing ? HID_EVENT_KEY_RELEASE : HID_EVENT_KEY_PRESS,
        .timestamp = hpet_get_timestamp()
    };
    event.key.keycode = key;
    event.key.scancode = byte;
    (void)hid_push_event(&event);

    extended = false;
    releasing = false;
}
