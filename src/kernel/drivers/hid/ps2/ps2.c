/**
 * @file ps2.c
 * @brief HID driver registration for the i8042-backed PS/2 devices.
 * @author friedrichOsDev
 */
#include <drivers/hid/ps2/ps2.h>

/**
 * @brief Handles ps2 driver init.
 * @return Result of the operation.
 */
static kerr_t ps2_driver_init() { return ps2_controller_init(); }
/**
 * @brief Handles ps2 driver deinit.
 */
static void ps2_driver_deinit() { ps2_controller_deinit(); }
/**
 * @brief Handles ps2 driver enable.
 */
static void ps2_driver_enable() { }
/**
 * @brief Handles ps2 driver disable.
 */
static void ps2_driver_disable() { ps2_controller_deinit(); }

static hid_driver_t ps2_hid_driver = {
    .driver_name = "i8042 PS/2 keyboard/mouse",
    .init = ps2_driver_init,
    .deinit = ps2_driver_deinit,
    .enable = ps2_driver_enable,
    .disable = ps2_driver_disable,
    .next = NULL
};

/**
 * @brief Handles ps2 register driver.
 * @return Result of the operation.
 */
kerr_t ps2_register_driver() {
    return hid_register_driver(&ps2_hid_driver);
}
