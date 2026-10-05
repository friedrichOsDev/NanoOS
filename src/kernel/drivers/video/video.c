/**
 * @file video.c
 * @brief Implementierung der Video-HAL.
 * @author friedrichOsDev
 */

#include <arch/x86_64/drivers/serial.h>
#include <drivers/video/video.h>
#include <stddef.h>

static const video_driver_ops_t *current_driver = NULL;
static bool video_subsystem_initialized = false;

bool video_register_driver(const video_driver_ops_t *ops) {
    if (!ops || !ops->init || !ops->get_framebuffer_address) {
        serial_printf(COM1, "VIDEO HAL: invalid driver\n");
        return false;
    }

    current_driver = ops;
    serial_printf(COM1, "VIDEO HAL: driver %s registered\n", ops->driver_name);
    return true;
}

bool video_init() {
    if (!current_driver) {
        serial_printf(COM1, "VIDEO HAL: no driver available\n");
        return false;
    }

    if (current_driver->init()) {
        video_subsystem_initialized = true;
        serial_printf(COM1, "VIDEO HAL: started %s\n", current_driver->driver_name);
        return true;
    }

    serial_printf(COM1, "VIDEO HAL: failed to initialize %s\n", current_driver->driver_name);
    return false;
}

void video_deinit() {
    if (current_driver && current_driver->deinit) {
        current_driver->deinit();
    }
    video_subsystem_initialized = false;
}

bool video_set_mode(uint32_t width, uint32_t height, uint32_t bpp) {
    if (!current_driver || !current_driver->set_mode) {
        return false;
    }

    video_mode_t mode = {
        .width = width,
        .height = height,
        .bpp = bpp,
        .pitch = width * (bpp / 8)};

    return current_driver->set_mode(mode);
}

bool video_get_mode(video_mode_t *out_mode) {
    if (!current_driver || !current_driver->get_mode || !out_mode) {
        return false;
    }
    return current_driver->get_mode(out_mode);
}

void video_flush(const void *src_buffer, uint64_t size) {
    if (!video_subsystem_initialized || !current_driver) {
        return;
    }

    if (current_driver->flush) {
        current_driver->flush(src_buffer, size);
    }
}

void *video_get_framebuffer_address() {
    if (!current_driver || !current_driver->get_framebuffer_address) {
        return NULL;
    }
    return current_driver->get_framebuffer_address();
}

const char *video_get_driver_name() {
    return current_driver ? current_driver->driver_name : "None";
}