/**
 * @file framebuffer.c
 * @brief Simple Framebuffer Driver
 * @author friedrichOsDev
 */

#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/memdef.h>
#include <core/scheduler.h>
#include <drivers/video/framebuffer/framebuffer.h>
#include <arch/x86_64/drivers/serial.h>
#include <core/init.h>
#include <stdint.h>

static backbuffer_info_t bb_info;
color_t black = {255, 0, 0, 0};
color_t white = {255, 255, 255, 255};
color_t red = {255, 255, 0, 0};
color_t green = {255, 0, 255, 0};
color_t blue = {255, 0, 0, 255};

void framebuffer_init_thread() {
    size_t backbuffer_size = kernel_fb_info.fb_height * kernel_fb_info.fb_width * sizeof(color_t);
    serial_printf(COM1, "FB: initializing framebuffer at %llx: %d:%d, %d bpp, pitch: %d, backbuffer_size: %d bytes\n", kernel_fb_info.fb_addr, kernel_fb_info.fb_width, kernel_fb_info.fb_height, kernel_fb_info.fb_bpp, kernel_fb_info.fb_pitch, backbuffer_size);
    bb_info.backbuffer = (uint8_t *)kzalloc(backbuffer_size);
    if (!bb_info.backbuffer) {
        serial_printf(COM1, "FB: failed to allocate backbuffer\n");
        return;
    }
    bb_info.backbuffer_size = backbuffer_size;

    serial_printf(COM1, "FB: Backbuffer Virt: %llx, Phys: %llx\n", (virt_addr_t)bb_info.backbuffer, (phys_addr_t)V2P((virt_addr_t)bb_info.backbuffer));

    while (1) {
        thread_yield();
    }
}