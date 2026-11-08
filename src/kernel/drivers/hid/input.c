/**
 * @file input.c
 * @brief Layout translation and stateful input facade above HID.
 */
#include <drivers/hid/input.h>
#include <kernel/sync.h>
#include <lib/util/str.h>

#define INPUT_MAX_LAYOUTS 8

static const keyboard_layout_t *layouts[INPUT_MAX_LAYOUTS];
static size_t layout_count;
static const keyboard_layout_t *active_layout;
static keyboard_state_t key_state;
static mouse_state_t pointer_state;
static spinlock_t input_lock = SPINLOCK_INIT;
static bool input_initialized;

static const keyboard_layout_t german_layout = {
    .name = "de-DE",
    .normal = {
        [KEY_1]='1', [KEY_2]='2', [KEY_3]='3', [KEY_4]='4', [KEY_5]='5',
        [KEY_6]='6', [KEY_7]='7', [KEY_8]='8', [KEY_9]='9', [KEY_0]='0',
        [KEY_Q]='q', [KEY_W]='w', [KEY_E]='e', [KEY_R]='r', [KEY_T]='t',
        [KEY_Z]='z', [KEY_U]='u', [KEY_I]='i', [KEY_O]='o', [KEY_P]='p',
        [KEY_A]='a', [KEY_S]='s', [KEY_D]='d', [KEY_F]='f', [KEY_G]='g',
        [KEY_H]='h', [KEY_J]='j', [KEY_K]='k', [KEY_L]='l', [KEY_Y]='y',
        [KEY_X]='x', [KEY_C]='c', [KEY_V]='v', [KEY_B]='b', [KEY_N]='n',
        [KEY_M]='m', [KEY_SPACE]=' ', [KEY_TAB]='\t', [KEY_RETURN]='\n',
        [KEY_BACK]='\b', [KEY_OEM1]=0x00FC, [KEY_OEM3]=0x00F6, [KEY_OEM7]=0x00E4,
        [KEY_OEM_PLUS]='+', [KEY_OEM4]=0x00DF, [KEY_OEM5]='#'
    },
    .shift = {
        [KEY_1]='!', [KEY_2]='"', [KEY_3]=0x00A7, [KEY_4]='$', [KEY_5]='%',
        [KEY_6]='&', [KEY_7]='/', [KEY_8]='(', [KEY_9]=')', [KEY_0]='=',
        [KEY_Q]='Q', [KEY_W]='W', [KEY_E]='E', [KEY_R]='R', [KEY_T]='T',
        [KEY_Z]='Z', [KEY_U]='U', [KEY_I]='I', [KEY_O]='O', [KEY_P]='P',
        [KEY_A]='A', [KEY_S]='S', [KEY_D]='D', [KEY_F]='F', [KEY_G]='G',
        [KEY_H]='H', [KEY_J]='J', [KEY_K]='K', [KEY_L]='L', [KEY_Y]='Y',
        [KEY_X]='X', [KEY_C]='C', [KEY_V]='V', [KEY_B]='B', [KEY_N]='N',
        [KEY_M]='M', [KEY_OEM1]=0x00DC, [KEY_OEM3]=0x00D6, [KEY_OEM7]=0x00C4,
        [KEY_OEM_PLUS]='*', [KEY_OEM4]='?', [KEY_OEM5]='\''
    }
};

static bool is_letter(hid_keycode_t key) {
    switch (key) {
    case KEY_Q: case KEY_W: case KEY_E: case KEY_R: case KEY_T:
    case KEY_Z: case KEY_U: case KEY_I: case KEY_O: case KEY_P:
    case KEY_A: case KEY_S: case KEY_D: case KEY_F: case KEY_G:
    case KEY_H: case KEY_J: case KEY_K: case KEY_L:
    case KEY_Y: case KEY_X: case KEY_C: case KEY_V: case KEY_B:
    case KEY_N: case KEY_M:
        return true;
    default:
        return false;
    }
}

static void update_modifiers(void) {
    key_state.shift = key_state.pressed[KEY_LSHIFT] || key_state.pressed[KEY_RSHIFT];
    key_state.control = key_state.pressed[KEY_LCONTROL] || key_state.pressed[KEY_RCONTROL];
    key_state.alt = key_state.pressed[KEY_LMENU];
    key_state.altgr = key_state.pressed[KEY_RMENU];
}

kerr_t keyboard_register_layout(const keyboard_layout_t *layout) {
    if (!layout || !layout->name || layout_count >= INPUT_MAX_LAYOUTS) return E_INVAL;
    for (size_t i = 0; i < layout_count; i++) {
        if (strcmp(layouts[i]->name, layout->name) == 0) return E_INVAL;
    }
    layouts[layout_count++] = layout;
    if (!active_layout) active_layout = layout;
    return E_OK;
}

kerr_t input_init(void) {
    if (input_initialized) return E_OK;
    memset(&key_state, 0, sizeof(key_state));
    memset(&pointer_state, 0, sizeof(pointer_state));
    kerr_t err = keyboard_register_layout(&german_layout);
    if (k_failed(err)) return err;
    input_initialized = true;
    return E_OK;
}

kerr_t keyboard_set_layout(const char *name) {
    if (!name) return E_INVAL;
    for (size_t i = 0; i < layout_count; i++) {
        if (strcmp(layouts[i]->name, name) == 0) {
            active_layout = layouts[i];
            return E_OK;
        }
    }
    return E_NOTFOUND;
}

const keyboard_layout_t *keyboard_get_layout(void) { return active_layout; }

uint32_t keyboard_translate_key(hid_keycode_t key, bool shift, bool altgr) {
    if (!active_layout || (size_t)key >= INPUT_KEY_COUNT) return 0;
    if (altgr && active_layout->altgr[key]) return active_layout->altgr[key];
    return shift ? active_layout->shift[key] : active_layout->normal[key];
}

static void update_state(const hid_event_t *event, input_event_t *out) {
    if (event->type == HID_EVENT_KEY_PRESS || event->type == HID_EVENT_KEY_RELEASE) {
        hid_keycode_t key = event->key.keycode;
        if ((size_t)key < INPUT_KEY_COUNT) {
            bool down = event->type == HID_EVENT_KEY_PRESS;
            bool was_down = key_state.pressed[key];
            key_state.pressed[key] = down;
            if (down && !was_down && key == KEY_CAPITAL) key_state.caps_lock = !key_state.caps_lock;
            if (down && !was_down && key == KEY_NUMLOCK) key_state.num_lock = !key_state.num_lock;
            update_modifiers();
            if (down) {
                bool shift = key_state.shift;
                if (is_letter(key) && key_state.caps_lock) shift = !shift;
                out->unicode = keyboard_translate_key(key, shift, key_state.altgr);
            }
        }
    } else if (event->type == HID_EVENT_MOUSE_MOVE) {
        pointer_state.x += event->mouse_move.dx;
        pointer_state.y += event->mouse_move.dy;
    } else if (event->type == HID_EVENT_MOUSE_BUTTON_PRESS) {
        pointer_state.buttons |= event->mouse_btn.button;
    } else if (event->type == HID_EVENT_MOUSE_BUTTON_RELEASE) {
        pointer_state.buttons &= (uint8_t)~event->mouse_btn.button;
    }
}

kerr_t input_poll_event(input_event_t *event) {
    if (!event || !input_initialized) return E_INVAL;
    memset(event, 0, sizeof(*event));
    kerr_t err = hid_poll_event(&event->hid);
    if (k_failed(err)) return err;
    irq_flags_t flags = spinlock_acquire_irqsave(&input_lock);
    update_state(&event->hid, event);
    spinlock_release_irqrestore(&input_lock, flags);
    return E_OK;
}

kerr_t keyboard_get_state(keyboard_state_t *out) {
    if (!out || !input_initialized) return E_INVAL;
    irq_flags_t flags = spinlock_acquire_irqsave(&input_lock);
    *out = key_state;
    spinlock_release_irqrestore(&input_lock, flags);
    return E_OK;
}

kerr_t mouse_get_state(mouse_state_t *out) {
    if (!out || !input_initialized) return E_INVAL;
    irq_flags_t flags = spinlock_acquire_irqsave(&input_lock);
    *out = pointer_state;
    spinlock_release_irqrestore(&input_lock, flags);
    return E_OK;
}
