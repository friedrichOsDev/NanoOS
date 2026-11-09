/**
 * @file ps2.c
 * @brief HID driver registration for the i8042-backed PS/2 devices.
 * @author friedrichOsDev
 */

#include <drivers/hid/ps2/ps2.h>

/**
 * @brief Initialize the registered PS/2 controller.
 *
 * @return E_OK on success, E error on failure.
 */
static kerr_t ps2_driver_init() { return ps2_controller_init(); }

/** @brief Deinitialize the registered PS/2 controller. */
static void ps2_driver_deinit() { ps2_controller_deinit(); }

/** @brief Enable the PS/2 HID driver. */
static void ps2_driver_enable() {}

/** @brief Disable the PS/2 HID driver. */
static void ps2_driver_disable() { ps2_controller_deinit(); }

static hid_driver_t ps2_hid_driver = {
    .driver_name = "i8042 PS/2 keyboard/mouse",
    .init = ps2_driver_init,
    .deinit = ps2_driver_deinit,
    .enable = ps2_driver_enable,
    .disable = ps2_driver_disable,
    .next = NULL};

/**
 * @brief Registers the PS2 driver.
 *
 * @return E_OK on success, E error on failure.
 */
kerr_t ps2_register_driver() {
    return hid_register_driver(&ps2_hid_driver);
}
