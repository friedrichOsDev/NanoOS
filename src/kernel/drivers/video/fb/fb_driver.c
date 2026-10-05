/**
 * @file fb_driver.c
 * @brief Implementierung des Framebuffer-Treibers.
 * @author friedrichOsDev
 */

#include <arch/x86_64/drivers/serial.h>
#include <core/init.h>
#include <drivers/video/fb/fb_driver.h>
#include <drivers/video/video.h>
#include <lib/string.h>

static bool generic_fb_init() {
    if (kernel_fb_info.fb_addr == 0) {
        serial_printf(COM1, "FB: framebuffer address missing\n");
        return false;
    }
    return true;
}

static void generic_fb_deinit() {}

static bool generic_fb_get_mode(video_mode_t *out_mode) {
    if (!out_mode)
        return false;

    out_mode->width = kernel_fb_info.fb_width;
    out_mode->height = kernel_fb_info.fb_height;
    out_mode->pitch = kernel_fb_info.fb_pitch;
    out_mode->bpp = kernel_fb_info.fb_bpp;
    return true;
}

static void *generic_fb_get_address() {
    return (void *)kernel_fb_info.fb_addr;
}

static void generic_fb_flush(const void *src_buffer, uint64_t size) {
    if (!src_buffer || kernel_fb_info.fb_addr == 0)
        return;

    void *dest = (void *)kernel_fb_info.fb_addr;
    memcpy(dest, src_buffer, size);
}

static const video_driver_ops_t generic_fb_ops = {
    .driver_name = "framebuffer",
    .init = generic_fb_init,
    .deinit = generic_fb_deinit,
    .set_mode = NULL, // No Support for mode setting
    .get_mode = generic_fb_get_mode,
    .get_framebuffer_address = generic_fb_get_address,
    .flush = generic_fb_flush,
};

bool generic_fb_driver_register() {
    return video_register_driver(&generic_fb_ops);
}