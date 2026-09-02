/**
 * @file framebuffer.c
 * @brief Simple Framebuffer Driver
 * @author friedrichOsDev
 */

#include <core/scheduler.h>
#include <drivers/video/framebuffer/framebuffer.h>
#include <arch/x86_64/drivers/serial.h>

void framebuffer_init_thread() {
    while (1) {
        thread_yield();
    }
}