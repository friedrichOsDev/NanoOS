/**
 * @file gui.c
 * @brief temporary GUI for kernel
 * @author friedrichOsDev
 */

#include <arch/x86_64/drivers/serial.h>
#include <core/gui.h>
#include <core/scheduler.h>
#include <drivers/video/framebuffer/framebuffer.h>
#include <lib/math/math.h>

void window_thread(void *arg) {
    (void)arg;
    layer_t *my_window = layer_create(400, 300, 11);
    if (!my_window) {
        serial_printf(COM1, "GUI: Failed to create window layer!\n");
        scheduler_thread_exit();
    }

    canvas_clear(&my_window->canvas, MAKE_COLOR_ALPHA(40, 40, 40, 230));
    canvas_draw_rectangle(&my_window->canvas, 0, 0, 400, 25, red, true, 0);
    canvas_draw_circle(&my_window->canvas, 200, 150, 50, blue, true, 0);

    double angle = 0.0;
    int64_t x;
    int64_t y;

    while (1) {
        x = 300 + (int64_t)(cos(angle) * 150.0);
        y = 200 + (int64_t)(sin(angle) * 100.0);

        serial_printf(COM1, "GUI: Drawing point (%d, %d)\n", x, y);

        layer_set_position(my_window, x, y);

        angle += 0.03;
        if (angle >= M_PI * 2) {
            angle = 0.0;
        }

        thread_sleep_ms(16);
    }
}

void window_thread2(void *arg) {
    (void)arg;
    layer_t *my_window = layer_create(400, 300, 10);
    if (!my_window) {
        serial_printf(COM1, "GUI: Failed to create window layer!\n");
        scheduler_thread_exit();
    }

    canvas_clear(&my_window->canvas, MAKE_COLOR(40, 40, 40));
    canvas_draw_rectangle(&my_window->canvas, 0, 0, 400, 25, red, true, 0);
    canvas_draw_circle(&my_window->canvas, 200, 150, 50, blue, true, 0);

    int64_t x = 300;
    int64_t dir = 2;

    while (1) {
        x += dir;
        if (x > 500 || x < 100)
            dir = -dir;

        layer_set_position(my_window, x, 200);
        thread_sleep_ms(16);
    }
}

void main_display_loop(void *arg) {
    (void)arg;
    while (1) {
        fb_render_scene();
        thread_sleep_ms(16); // ~60 FPS
    }
}