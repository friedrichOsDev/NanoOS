/**
 * @file ps2.c
 * @brief HID driver registration for the i8042-backed PS/2 devices.
 */
#include <drivers/hid/ps2/ps2.h>

static kerr_t ps2_driver_init(void) { return ps2_controller_init(); }
static void ps2_driver_deinit(void) { ps2_controller_deinit(); }
static void ps2_driver_enable(void) { }
static void ps2_driver_disable(void) { ps2_controller_deinit(); }

static hid_driver_t ps2_hid_driver = {
    .driver_name = "i8042 PS/2 keyboard/mouse",
    .init = ps2_driver_init,
    .deinit = ps2_driver_deinit,
    .enable = ps2_driver_enable,
    .disable = ps2_driver_disable,
    .next = NULL
};

kerr_t ps2_register_driver(void) {
    return hid_register_driver(&ps2_hid_driver);
}
