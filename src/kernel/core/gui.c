/**
 * @file gui.c
 * @brief temporary GUI for kernel
 * @author friedrichOsDev
 */

#include <core/gui.h>
#include <core/scheduler.h>
#include <drivers/video/framebuffer/framebuffer.h>

void window_thread(void *arg) {
    (void)arg;
    layer_t *my_window = layer_create(400, 300, 11);
    layer_set_position(my_window, 100, 100);

    while (1) {
        canvas_clear(&my_window->canvas, MAKE_COLOR(40, 40, 40));
        canvas_draw_rectangle(&my_window->canvas, 0, 0, 400, 25, red, true, 0);
        canvas_draw_circle(&my_window->canvas, 200, 150, 50, blue, true, 0);

        thread_sleep_ms(16);
    }
}

void window_thread2(void *arg) {
    (void)arg;
    layer_t *my_window = layer_create(400, 300, 10);
    layer_set_position(my_window, 300, 300);

    while (1) {
        canvas_clear(&my_window->canvas, MAKE_COLOR(40, 40, 40));
        canvas_draw_rectangle(&my_window->canvas, 0, 0, 400, 25, red, true, 0);
        canvas_draw_circle(&my_window->canvas, 200, 150, 50, blue, true, 0);

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