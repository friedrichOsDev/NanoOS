/**
 * @file init.c
 * @brief Kernel initialization
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <arch/multiboot2.h>
#include <drivers/gfx/compositor.h>
#include <drivers/gfx/gfx.h>
#include <drivers/hid/hid.h>
#include <drivers/hid/input.h>
#include <drivers/hid/ps2/ps2.h>
#include <drivers/pci.h>
#include <drivers/rtc.h>
#include <kernel/panic.h>
#include <lib/print.h>
#include <lib/util/str.h>
#include <mm/memory.h>
#include <pch.h>
#include <sched/process.h>
#include <sched/sched.h>
#include <sched/taskmgr.h>
#include <sched/thread.h>

void kernel_shutdown();

/**
 * @brief Handle kernel-wide input shortcuts.
 *
 * @param[in] event Translated input event.
 * @param[in] context Unused callback context.
 */
static void kernel_input_handler(const input_event_t *event, void *context) {
    (void)context;

    if (event->hid.type == HID_EVENT_KEY_PRESS &&
        event->hid.key.keycode == KEY_ESC &&
        event->key_action == INPUT_KEY_ACTION_PRESSED) {
        log_info("ESC pressed, shutting down");
        kernel_shutdown();
        log_warn("shutdown request returned");
    }
}

/**
 * @brief Helper to render a styled window frame with titlebar and controls.
 *
 * @param[in,out] canvas Target canvas.
 * @param[in] title Window title text.
 * @param[in] bg_color Background fill color.
 * @param[in] title_bar_color Titlebar fill color.
 */
static void draw_window_frame(canvas_t *canvas, const char *title, color_t bg_color, color_t title_bar_color) {
    rect_size_t size = canvas->size;

    canvas_clear(canvas, bg_color);
    canvas_draw_rectangle(canvas, (point_t){0, 0}, (rect_size_t){size.width, 24}, title_bar_color, true, 0);

    /* Window control buttons */
    canvas_draw_circle(canvas, (point_t){12, 12}, 4, COLOR_RED, true, 0);
    canvas_draw_circle(canvas, (point_t){24, 12}, 4, COLOR_YELLOW, true, 0);
    canvas_draw_circle(canvas, (point_t){36, 12}, 4, COLOR_GREEN, true, 0);

    /* Title text */
    canvas_draw_text(canvas, (point_t){48, 6}, title, COLOR_WHITE, 1);

    /* 1px border */
    canvas_draw_rectangle(canvas, (point_t){0, 0}, size, MAKE_COLOR_ALPHA(120, 160, 200, 180), false, 1);
}

/**
 * @brief Initialize kernel drivers and process input events.
 *
 * @param[in] arg Unused argument.
 */
static void kernel_init_thread(void *arg) {
    (void)arg;

    kerr_t err = gfx_init();
    if (k_failed(err)) {
        log_warn("gfx init failed, no graphical output available (%s)", kerr_str(err));
    }
    log_info("gfx done");

    err = hid_init();
    if (k_failed(err)) {
        log_warn("hid initialization failed (%s)", kerr_str(err));
        for (;;) {
            thread_yield();
        }
    }
    log_info("hid done");

    err = input_init();
    if (k_failed(err)) {
        log_warn("input initialization failed (%s)", kerr_str(err));
        for (;;) {
            thread_yield();
        }
    }

    err = ps2_register_driver();
    if (k_failed(err)) {
        log_warn("PS/2 driver unavailable (%s)", kerr_str(err));
    }
    log_info("input initialization done");

    err = input_register_handler(kernel_input_handler, NULL);
    if (k_failed(err)) {
        log_error("failed to register kernel input handler (%s)", kerr_str(err));
    }

    err = input_dispatcher_start();
    if (k_failed(err)) {
        log_error("failed to start input dispatcher (%s)", kerr_str(err));
        for (;;) {
            thread_yield();
        }
    }
    log_info("input dispatcher started");

    uint64_t screen_w = compositor_get_width();
    uint64_t screen_h = compositor_get_height();
    if (screen_w == 0 || screen_h == 0) {
        screen_w = 1024;
        screen_h = 768;
    }

    /* 1. Desktop Background Layer (LAYER_Z_BACKGROUND) */
    layer_t *bg_layer = NULL;
    layer_create((rect_size_t){screen_w, screen_h}, LAYER_Z_BACKGROUND, &bg_layer);
    layer_set_opaque(bg_layer, true);
    layer_draw_begin(bg_layer);
    canvas_clear(&bg_layer->canvas, MAKE_COLOR(18, 24, 38));

    /* Faint desktop grid */
    for (uint64_t gx = 0; gx < screen_w; gx += 40) {
        canvas_draw_line(&bg_layer->canvas, (point_t){(int64_t)gx, 30}, (point_t){(int64_t)gx, (int64_t)screen_h - 32}, MAKE_COLOR(24, 32, 48), 1);
    }
    for (uint64_t gy = 30; gy < screen_h - 30; gy += 40) {
        canvas_draw_line(&bg_layer->canvas, (point_t){0, (int64_t)gy}, (point_t){(int64_t)screen_w - 1, (int64_t)gy}, MAKE_COLOR(24, 32, 48), 1);
    }

    /* Top menu bar */
    canvas_draw_rectangle(&bg_layer->canvas, (point_t){0, 0}, (rect_size_t){screen_w, 28}, MAKE_COLOR(28, 36, 52), true, 0);
    canvas_draw_text(&bg_layer->canvas, (point_t){14, 7}, "NanoOS v0.0.3", COLOR_CYAN, 1);
    canvas_draw_text(&bg_layer->canvas, (point_t){(int64_t)(screen_w / 2 - 130), 7}, "Compositor & 2D Canvas Showcase", COLOR_WHITE, 1);
    canvas_draw_text(&bg_layer->canvas, (point_t){(int64_t)(screen_w - 150), 7}, "[ESC] Shutdown", COLOR_YELLOW, 1);

    /* Bottom taskbar */
    canvas_draw_rectangle(&bg_layer->canvas, (point_t){0, (int64_t)screen_h - 30}, (rect_size_t){screen_w, 30}, MAKE_COLOR(24, 30, 44), true, 0);
    canvas_draw_text(&bg_layer->canvas, (point_t){16, (int64_t)screen_h - 22}, "[NanoOS Menu]   [Terminal]   [Compositor Demo]   [Tasks]", COLOR_LIGHT_GRAY, 1);
    layer_draw_end(bg_layer);

    /* 2. Window 1: 2D Canvas Primitives (LAYER_Z_WINDOW) */
    layer_t *win1 = NULL;
    layer_create((rect_size_t){340, 260}, LAYER_Z_WINDOW, &win1);
    layer_set_position(win1, (point_t){35, 45});
    layer_draw_begin(win1);
    draw_window_frame(&win1->canvas, "Canvas 2D Primitives", MAKE_COLOR_ALPHA(22, 28, 40, 230), MAKE_COLOR_ALPHA(35, 48, 68, 255));

    /* Triangles */
    canvas_draw_triangle(&win1->canvas, (point_t){30, 40}, (point_t){100, 110}, (point_t){30, 110}, MAKE_COLOR_ALPHA(0, 220, 220, 200), true, 0);
    canvas_draw_triangle(&win1->canvas, (point_t){130, 40}, (point_t){200, 110}, (point_t){150, 120}, COLOR_YELLOW, false, 2);

    /* Circles */
    canvas_draw_circle(&win1->canvas, (point_t){260, 80}, 35, COLOR_ORANGE, false, 3);
    canvas_draw_circle(&win1->canvas, (point_t){260, 80}, 18, MAKE_COLOR_ALPHA(255, 80, 80, 210), true, 0);

    /* Optimized thick lines */
    canvas_draw_line(&win1->canvas, (point_t){30, 150}, (point_t){180, 220}, MAKE_COLOR_ALPHA(50, 220, 120, 220), 6);
    canvas_draw_line(&win1->canvas, (point_t){30, 220}, (point_t){180, 150}, MAKE_COLOR_ALPHA(200, 80, 230, 220), 4);

    canvas_draw_text(&win1->canvas, (point_t){205, 170}, "Polygon Lines", COLOR_WHITE, 1);
    canvas_draw_text(&win1->canvas, (point_t){205, 190}, "Zero Overdraw", COLOR_GREEN, 1);
    layer_draw_end(win1);

    /* 3. Window 2: Alpha & Glass Blending (LAYER_Z_WINDOW + 10) - overlaps Window 1 */
    layer_t *win2 = NULL;
    layer_create((rect_size_t){360, 270}, LAYER_Z_WINDOW + 10, &win2);
    layer_set_position(win2, (point_t){240, 130});
    layer_draw_begin(win2);
    draw_window_frame(&win2->canvas, "Alpha & Glass Blending", MAKE_COLOR_ALPHA(45, 25, 60, 180), MAKE_COLOR_ALPHA(75, 40, 100, 220));

    /* Overlapping translucent primary color circles */
    canvas_draw_circle(&win2->canvas, (point_t){90, 85}, 32, MAKE_COLOR_ALPHA(255, 50, 50, 150), true, 0);
    canvas_draw_circle(&win2->canvas, (point_t){130, 85}, 32, MAKE_COLOR_ALPHA(50, 255, 50, 150), true, 0);
    canvas_draw_circle(&win2->canvas, (point_t){110, 120}, 32, MAKE_COLOR_ALPHA(50, 120, 255, 150), true, 0);

    canvas_draw_text(&win2->canvas, (point_t){190, 75}, "RGB Blend Circles", COLOR_WHITE, 1);
    canvas_draw_text(&win2->canvas, (point_t){190, 95}, "Multi-Layer Alpha", COLOR_MAGENTA, 1);

    /* Alpha step gradient bars */
    canvas_draw_text(&win2->canvas, (point_t){25, 175}, "Alpha Opacity Steps:", COLOR_LIGHT_GRAY, 1);
    for (int i = 0; i < 5; i++) {
        uint8_t a = (uint8_t)(50 + i * 50);
        point_t bar_pos = {25 + i * 60, 195};
        canvas_draw_rectangle(&win2->canvas, bar_pos, (rect_size_t){50, 25}, MAKE_COLOR_ALPHA(0, 210, 255, a), true, 0);
        canvas_draw_rectangle(&win2->canvas, bar_pos, (rect_size_t){50, 25}, COLOR_WHITE, false, 1);
    }
    layer_draw_end(win2);

    /* 4. Window 3: Viewport Clipping Demo (LAYER_Z_WINDOW + 20) */
    layer_t *win3 = NULL;
    int64_t win3_x = (int64_t)screen_w - 340;
    if (win3_x < 300) {
        win3_x = 300;
    }
    layer_create((rect_size_t){310, 230}, LAYER_Z_WINDOW + 20, &win3);
    layer_set_position(win3, (point_t){win3_x, 100});
    layer_draw_begin(win3);
    draw_window_frame(&win3->canvas, "Viewport Clipping Demo", MAKE_COLOR_ALPHA(20, 36, 32, 220), MAKE_COLOR_ALPHA(32, 60, 50, 255));

    /* Inner clipped container widget */
    point_t clip_box_pos = {25, 45};
    rect_size_t clip_box_size = {260, 140};
    canvas_draw_rectangle(&win3->canvas, clip_box_pos, clip_box_size, MAKE_COLOR_ALPHA(10, 20, 18, 255), true, 0);
    canvas_draw_rectangle(&win3->canvas, clip_box_pos, clip_box_size, COLOR_GRAY, false, 1);

    /* Set active clipping region */
    canvas_set_clip_rect(&win3->canvas, (rect_t){clip_box_pos, clip_box_size});

    /* Draw elements that extend outside the clip bounds to showcase clipping */
    canvas_draw_circle(&win3->canvas, (point_t){25, 45}, 45, COLOR_RED, true, 0);
    canvas_draw_circle(&win3->canvas, (point_t){285, 185}, 50, COLOR_BLUE, true, 0);
    canvas_draw_line(&win3->canvas, (point_t){10, 170}, (point_t){300, 30}, COLOR_YELLOW, 5);
    canvas_draw_text(&win3->canvas, (point_t){40, 100}, "Clipped to widget rect!", COLOR_GREEN, 1);

    /* Reset clip rect */
    canvas_reset_clip_rect(&win3->canvas);
    canvas_draw_text(&win3->canvas, (point_t){25, 195}, "Inner region bounded by clip_rect", COLOR_LIGHT_GRAY, 1);
    layer_draw_end(win3);

    /* 5. Translucent Bottom Dock Panel (LAYER_Z_OVERLAY) */
    layer_t *dock = NULL;
    int64_t dock_w = 460;
    int64_t dock_x = (int64_t)(screen_w - (uint64_t)dock_w) / 2;
    int64_t dock_y = (int64_t)screen_h - 100;
    layer_create((rect_size_t){(uint64_t)dock_w, 60}, LAYER_Z_OVERLAY, &dock);
    layer_set_position(dock, (point_t){dock_x, dock_y});
    layer_draw_begin(dock);
    canvas_clear(&dock->canvas, COLOR_TRANSPARENT);
    canvas_draw_rectangle(&dock->canvas, (point_t){0, 0}, (rect_size_t){(uint64_t)dock_w, 60}, MAKE_COLOR_ALPHA(14, 20, 32, 190), true, 0);
    canvas_draw_rectangle(&dock->canvas, (point_t){0, 0}, (rect_size_t){(uint64_t)dock_w, 60}, MAKE_COLOR_ALPHA(90, 160, 240, 210), false, 1);

    color_t icon_colors[5] = {COLOR_CYAN, COLOR_ORANGE, COLOR_GREEN, COLOR_MAGENTA, COLOR_YELLOW};
    const char *icon_labels[5] = {"SYS", "GFX", "NET", "APP", "CFG"};
    for (int i = 0; i < 5; i++) {
        point_t icenter = {45 + i * 90, 30};
        canvas_draw_circle(&dock->canvas, icenter, 20, MAKE_COLOR_ALPHA(icon_colors[i].r, icon_colors[i].g, icon_colors[i].b, 170), true, 0);
        canvas_draw_circle(&dock->canvas, icenter, 20, COLOR_WHITE, false, 1);
        canvas_draw_text(&dock->canvas, (point_t){icenter.x - 12, icenter.y - 7}, icon_labels[i], COLOR_WHITE, 1);
    }
    layer_draw_end(dock);

    /* 6. Live Performance & FPS HUD Layer (LAYER_Z_OVERLAY + 10) */
    layer_t *fps_layer = NULL;
    int64_t fps_x = (int64_t)screen_w - 225;
    layer_create((rect_size_t){215, 95}, LAYER_Z_OVERLAY + 10, &fps_layer);
    layer_set_position(fps_layer, (point_t){fps_x, 35});

    /* 7. Animated Floating Orb Cursor Sprite (LAYER_Z_CURSOR) */
    layer_t *cursor = NULL;
    layer_create((rect_size_t){44, 44}, LAYER_Z_CURSOR, &cursor);
    layer_draw_begin(cursor);
    canvas_clear(&cursor->canvas, COLOR_TRANSPARENT);
    canvas_draw_circle(&cursor->canvas, (point_t){22, 22}, 20, MAKE_COLOR_ALPHA(0, 210, 255, 70), true, 0);
    canvas_draw_circle(&cursor->canvas, (point_t){22, 22}, 14, MAKE_COLOR_ALPHA(0, 230, 255, 140), true, 0);
    canvas_draw_circle(&cursor->canvas, (point_t){22, 22}, 7, MAKE_COLOR_ALPHA(255, 255, 255, 240), true, 0);
    canvas_draw_line(&cursor->canvas, (point_t){22, 0}, (point_t){22, 43}, MAKE_COLOR_ALPHA(255, 255, 255, 180), 1);
    canvas_draw_line(&cursor->canvas, (point_t){0, 22}, (point_t){43, 22}, MAKE_COLOR_ALPHA(255, 255, 255, 180), 1);
    layer_draw_end(cursor);

    /* Animation & Update Loop */
    int64_t orb_x = 120;
    int64_t orb_y = 180;
    int64_t orb_dx = 3;
    int64_t orb_dy = 2;
    uint64_t loop_frames = 0;

    for (;;) {
        orb_x += orb_dx;
        orb_y += orb_dy;
        if (orb_x <= 10 || orb_x + 44 >= (int64_t)screen_w - 10) {
            orb_dx = -orb_dx;
        }
        if (orb_y <= 35 || orb_y + 44 >= (int64_t)screen_h - 35) {
            orb_dy = -orb_dy;
        }
        layer_set_position(cursor, (point_t){orb_x, orb_y});

        loop_frames++;
        if ((loop_frames % 4) == 0) {
            layer_draw_begin(fps_layer);
            canvas_clear(&fps_layer->canvas, COLOR_TRANSPARENT);
            canvas_draw_rectangle(&fps_layer->canvas, (point_t){0, 0}, (rect_size_t){215, 95}, MAKE_COLOR_ALPHA(12, 16, 26, 230), true, 0);
            canvas_draw_rectangle(&fps_layer->canvas, (point_t){0, 0}, (rect_size_t){215, 95}, COLOR_GREEN, false, 1);

            canvas_draw_text(&fps_layer->canvas, (point_t){12, 8}, "PERFORMANCE HUD", COLOR_WHITE, 1);

            char buf[32];
            uint64_t fps = compositor_get_current_fps();
            snprintf(buf, sizeof(buf), "FPS:   %llu", (unsigned long long)fps);
            canvas_draw_text(&fps_layer->canvas, (point_t){12, 28}, buf, COLOR_GREEN, 1);

            double dt_ms = compositor_get_delta_time() * 1000.0;
            snprintf(buf, sizeof(buf), "Delta: %.2f ms", dt_ms);
            canvas_draw_text(&fps_layer->canvas, (point_t){12, 48}, buf, COLOR_YELLOW, 1);

            snprintf(buf, sizeof(buf), "Frame: %llu", (unsigned long long)loop_frames);
            canvas_draw_text(&fps_layer->canvas, (point_t){12, 68}, buf, COLOR_CYAN, 1);

            layer_draw_end(fps_layer);
        }

        log_debug("wait 16ms");
        thread_sleep_ms(16);
    }

    thread_exit();
}

/**
 * @brief Initialize the kernel and start the scheduler.
 *
 * @param[in] magic Multiboot2 magic number.
 * @param[in] info_ptr Pointer to the multiboot2 information structure.
 */
void kernel_init(const uint64_t magic, const uint64_t info_ptr); // first declaration
void kernel_init(const uint64_t magic, const uint64_t info_ptr) {
    serial_init();
    log_info("start");

    gdt_init();
    log_info("gdt done");
    idt_init();
    log_info("idt done");
    irq_init();
    log_info("irq done");
    fpu_init();
    log_info("fpu done");

    arch_sti();
    log_info("interrupts enabled");

    multiboot_parse(magic, info_ptr);
    log_info("multiboot2 done");

    K_TRY_PANIC(pmm_init());
    log_info("pmm done");
    K_TRY_PANIC(vmm_init());
    log_info("vmm done");
    K_TRY_PANIC(heap_init());
    log_info("heap done");

    virt_addr_t rsdp_addr = get_rsdp_addr();
    k_assert(rsdp_addr != 0);
    K_TRY_PANIC(acpi_init(rsdp_addr));
    log_info("acpi done");

    K_TRY_PANIC(apic_init());
    log_info("apic done");
    hpet_init();
    log_info("hpet done");
    rtc_init();
    log_info("rtc done");

    K_TRY_PANIC(pci_init());
    log_info("pci done");

    log_debug("lapic_target_hz=%d", 1000);
    lapic_init(1000);
    log_info("lapic done");

    K_TRY_PANIC(sched_init());
    log_info("sched done");

    K_TRY_PANIC(smp_init());
    log_info("smp done");

    log_info("multi-core scheduling running");

    thread_t *kernel_init_th = NULL;
    K_TRY_PANIC(thread_create_on_cpu(proc_get_kernel_proc(), kernel_init_thread, NULL, "kernel_init", -1, &kernel_init_th));
    log_debug("kernel_init_thread created");

    sched_enable();
    log_info("sched enabled");

    thread_exit();

    log_panic("no code schould be executed after thread_exit()");

    while (1) {
        arch_hlt();
    }
}

/** @brief Clean kernel shutdown */
void kernel_shutdown() {
    acpi_power_off();
}
