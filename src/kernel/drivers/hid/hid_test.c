/**
 * @file hid_test.c
 * @brief Human-readable HID event diagnostics.
 */
#include <drivers/hid/hid_test.h>
#include <drivers/hid/input.h>
#include <lib/log.h>
#include <pch.h>

static const char *const key_names[INPUT_KEY_COUNT] = {
    [KEY_UNKNOWN] = "UNKNOWN",
    [KEY_ESC] = "ESC",
    [KEY_F1] = "F1", [KEY_F2] = "F2", [KEY_F3] = "F3",
    [KEY_F4] = "F4", [KEY_F5] = "F5", [KEY_F6] = "F6",
    [KEY_F7] = "F7", [KEY_F8] = "F8", [KEY_F9] = "F9",
    [KEY_F10] = "F10", [KEY_F11] = "F11", [KEY_F12] = "F12",
    [KEY_SNAPSHOT] = "PRINT_SCREEN", [KEY_SCROLL] = "SCROLL_LOCK",
    [KEY_PAUSE] = "PAUSE",
    [KEY_OEM5] = "OEM5",
    [KEY_1] = "1", [KEY_2] = "2", [KEY_3] = "3", [KEY_4] = "4",
    [KEY_5] = "5", [KEY_6] = "6", [KEY_7] = "7", [KEY_8] = "8",
    [KEY_9] = "9", [KEY_0] = "0",
    [KEY_OEM4] = "OEM4", [KEY_OEM6] = "OEM6",
    [KEY_BACK] = "BACKSPACE",
    [KEY_INSERT] = "INSERT", [KEY_HOME] = "HOME", [KEY_PRIOR] = "PAGE_UP",
    [KEY_NUMLOCK] = "NUM_LOCK", [KEY_NUMLOCK_DIVIDE] = "NUM_DIVIDE",
    [KEY_NUMLOCK_MULTIPLY] = "NUM_MULTIPLY", [KEY_NUMLOCK_SUBTRACT] = "NUM_SUBTRACT",
    [KEY_TAB] = "TAB",
    [KEY_Q] = "Q", [KEY_W] = "W", [KEY_E] = "E", [KEY_R] = "R",
    [KEY_T] = "T", [KEY_Z] = "Z", [KEY_U] = "U", [KEY_I] = "I",
    [KEY_O] = "O", [KEY_P] = "P",
    [KEY_OEM1] = "OEM1", [KEY_OEM_PLUS] = "OEM_PLUS",
    [KEY_RETURN] = "ENTER", [KEY_DELETE] = "DELETE", [KEY_END] = "END",
    [KEY_NEXT] = "PAGE_DOWN", [KEY_NUMLOCK_HOME] = "NUM_HOME",
    [KEY_NUMLOCK_UP] = "NUM_UP", [KEY_NUMLOCK_PRIOR] = "NUM_PAGE_UP",
    [KEY_NUMLOCK_ADD] = "NUM_ADD",
    [KEY_CAPITAL] = "CAPS_LOCK",
    [KEY_A] = "A", [KEY_S] = "S", [KEY_D] = "D", [KEY_F] = "F",
    [KEY_G] = "G", [KEY_H] = "H", [KEY_J] = "J", [KEY_K] = "K",
    [KEY_L] = "L", [KEY_OEM3] = "OEM3", [KEY_OEM7] = "OEM7",
    [KEY_OEM2] = "OEM2",
    [KEY_NUMLOCK_LEFT] = "NUM_LEFT", [KEY_NUMLOCK_CLEAR] = "NUM_CLEAR",
    [KEY_NUMLOCK_RIGHT] = "NUM_RIGHT",
    [KEY_LSHIFT] = "LEFT_SHIFT", [KEY_OEM10] = "OEM10",
    [KEY_Y] = "Y", [KEY_X] = "X", [KEY_C] = "C", [KEY_V] = "V",
    [KEY_B] = "B", [KEY_N] = "N", [KEY_M] = "M",
    [KEY_OEM_CC] = "OEM_CC", [KEY_OEM_PE] = "OEM_PE",
    [KEY_OEM_MI] = "OEM_MI", [KEY_RSHIFT] = "RIGHT_SHIFT",
    [KEY_NUMLOCK_END] = "NUM_END", [KEY_NUMLOCK_DOWN] = "NUM_DOWN",
    [KEY_NUMLOCK_NEXT] = "NUM_PAGE_DOWN", [KEY_NUMLOCK_RETURN] = "NUM_ENTER",
    [KEY_LCONTROL] = "LEFT_CTRL", [KEY_LWIN] = "LEFT_META",
    [KEY_LMENU] = "LEFT_ALT", [KEY_SPACE] = "SPACE",
    [KEY_RMENU] = "RIGHT_ALT/ALTGR", [KEY_RWIN] = "RIGHT_META",
    [KEY_APPS] = "MENU", [KEY_RCONTROL] = "RIGHT_CTRL",
    [KEY_UP] = "UP", [KEY_LEFT] = "LEFT", [KEY_DOWN] = "DOWN",
    [KEY_RIGHT] = "RIGHT", [KEY_NUMLOCK_INSERT] = "NUM_INSERT",
    [KEY_NUMLOCK_DELETE] = "NUM_DELETE"
};

static const char *key_name(hid_keycode_t key) {
    size_t index = (size_t)key;
    if (index >= INPUT_KEY_COUNT || key_names[index] == NULL) return "UNMAPPED";
    return key_names[index];
}

static void log_key_event(const input_event_t *event) {
    const hid_event_t *hid = &event->hid;
    const char *action = hid->type == HID_EVENT_KEY_PRESS ? "DOWN" : "UP";
    log_debug("[HID][KEY] %s key=%s code=%u scancode=0x%02X unicode=U+%04X timestamp=%llu",
              action, key_name(hid->key.keycode), (unsigned)hid->key.keycode,
              (unsigned)hid->key.scancode, (unsigned)event->unicode,
              (unsigned long long)hid->timestamp);
}

static void log_input_event(const input_event_t *event) {
    const hid_event_t *hid = &event->hid;
    switch (hid->type) {
    case HID_EVENT_NONE:
        log_debug("[HID] NONE timestamp=%llu", (unsigned long long)hid->timestamp);
        break;
    case HID_EVENT_KEY_PRESS:
    case HID_EVENT_KEY_RELEASE:
        log_key_event(event);
        break;
    case HID_EVENT_MOUSE_MOVE:
        log_debug("[HID][MOUSE] MOVE dx=%lld dy=%lld timestamp=%llu",
                  (long long)hid->mouse_move.dx, (long long)hid->mouse_move.dy,
                  (unsigned long long)hid->timestamp);
        break;
    case HID_EVENT_MOUSE_BUTTON_PRESS:
        log_debug("[HID][MOUSE] BUTTON_DOWN mask=0x%02X timestamp=%llu",
                  (unsigned)hid->mouse_btn.button, (unsigned long long)hid->timestamp);
        break;
    case HID_EVENT_MOUSE_BUTTON_RELEASE:
        log_debug("[HID][MOUSE] BUTTON_UP mask=0x%02X timestamp=%llu",
                  (unsigned)hid->mouse_btn.button, (unsigned long long)hid->timestamp);
        break;
    case HID_EVENT_MOUSE_SCROLL:
        log_debug("[HID][MOUSE] SCROLL delta=%lld timestamp=%llu",
                  (long long)hid->mouse_scroll.delta, (unsigned long long)hid->timestamp);
        break;
    }
}

size_t hid_test_poll(size_t max_events) {
    size_t processed = 0;
    while (processed < max_events) {
        input_event_t event;
        kerr_t err = input_poll_event(&event);
        if (err == E_NOTAVAIL) break;
        if (k_failed(err)) {
            log_warn("[HID_TEST] input_poll_event failed: %s", kerr_str(err));
            break;
        }
        log_input_event(&event);
        processed++;
    }
    return processed;
}
