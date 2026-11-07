/**
 * @file version_splash.c
 * @brief Version splash screen renderer for NanoOS
 * @author friedrichOsDev
 */

#include <drivers/gfx/compositor.h>
#include <drivers/gfx/gfx.h>
#include <drivers/gfx/version_splash.h>
#include <lib/util/str.h>
#include <pch.h>
#include <sched/sched.h>

/** @brief Embedded 5x5 pixel bitmap font array. */
static const uint8_t font_5x5[][5] = {
    // '0': Index 0
    {0b01110, 0b10011, 0b10101, 0b11001, 0b01110},
    // '1': Index 1
    {0b00100, 0b01100, 0b00100, 0b00100, 0b01110},
    // '2': Index 2
    {0b01110, 0b10001, 0b00110, 0b01000, 0b11111},
    // '3': Index 3
    {0b11110, 0b00001, 0b00110, 0b00001, 0b11110},
    // '4': Index 4
    {0b10001, 0b10001, 0b11111, 0b00001, 0b00001},
    // '5': Index 5
    {0b11111, 0b10000, 0b11110, 0b00001, 0b11110},
    // '6': Index 6
    {0b01110, 0b10000, 0b11110, 0b10001, 0b01110},
    // '7': Index 7
    {0b11111, 0b00010, 0b00100, 0b01000, 0b01000},
    // '8': Index 8
    {0b01110, 0b10001, 0b01110, 0b10001, 0b01110},
    // '9': Index 9
    {0b01110, 0b10001, 0b01111, 0b00001, 0b01110},
    // '.': Index 10
    {0b00000, 0b00000, 0b00000, 0b00000, 0b01000},
    // 'N': Index 11
    {0b10001, 0b11001, 0b10101, 0b10011, 0b10001},
    // 'a': Index 12
    {0b00000, 0b01110, 0b00001, 0b01111, 0b01101},
    // 'n': Index 13
    {0b00000, 0b11100, 0b10010, 0b10010, 0b10010},
    // 'o': Index 14
    {0b00000, 0b01110, 0b10001, 0b10001, 0b01110},
    // 'O': Index 15
    {0b01110, 0b10001, 0b10001, 0b10001, 0b01110},
    // 'S': Index 16
    {0b01111, 0b10000, 0b01110, 0b00001, 0b11110},
    // '-': Index 17
    {0b00000, 0b00000, 0b11111, 0b00000, 0b00000}};

/**
 * @brief Maps an ASCII character to its corresponding bitmap index in font_5x5.
 *
 * @param[in] c Character to look up.
 * @return Index into font_5x5 array on success, or -1 if character is unsupported.
 */
static int get_char_index(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c == '.')
        return 10;
    if (c == 'N')
        return 11;
    if (c == 'a')
        return 12;
    if (c == 'n')
        return 13;
    if (c == 'o')
        return 14;
    if (c == 'O')
        return 15;
    if (c == 'S')
        return 16;
    if (c == '-')
        return 17;
    return -1;
}

/**
 * @brief Draws a string onto a canvas using the scaled 5x5 bitmap font.
 *
 * @param[in,out] canvas Pointer to target drawing canvas structure.
 * @param[in] start_x Starting X-coordinate position in pixels.
 * @param[in] start_y Starting Y-coordinate position in pixels.
 * @param[in] str Null-terminated string to render.
 * @param[in] color Color value for the text pixels.
 * @param[in] scale Pixel scaling factor (width & height per bitmap pixel).
 */
static void draw_string(canvas_t *canvas, int64_t start_x, int64_t start_y, const char *str, color_t color, uint64_t scale) {
    int64_t cursor_x = start_x;

    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == ' ') {
            cursor_x += (int64_t)(4 * scale);
            continue;
        }

        int idx = get_char_index(str[i]);
        if (idx >= 0) {
            for (int row = 0; row < 5; row++) {
                uint8_t line = font_5x5[idx][row];
                for (int col = 0; col < 5; col++) {
                    if (line & (1 << (4 - col))) {
                        point_t pos = {
                            .x = cursor_x + (int64_t)((uint64_t)col * scale),
                            .y = start_y + (int64_t)((uint64_t)row * scale)};
                        rect_size_t pixel_box = {.width = scale, .height = scale};
                        canvas_draw_rectangle(canvas, pos, pixel_box, color, true, 0);
                    }
                }
            }
        }
        cursor_x += (int64_t)(6 * scale);
    }
}

/**
 * @brief Displays a version splash screen centered on screen for a given duration.
 *
 * @param[in] version_str Null-terminated version string (e.g., "0.0.3").
 * @param[in] duration_ms Duration in milliseconds to hold the splash display.
 * @return E_OK on success, E_INVAL if version_str is NULL, E_NOINIT if compositor screen size is zero.
 */
kerr_t version_splash_show(const char *version_str, uint64_t duration_ms) {
    if (!version_str) {
        return E_INVAL;
    }

    rect_size_t screen = compositor_get_size();
    if (screen.width == 0 || screen.height == 0) {
        return E_NOINIT;
    }

    layer_t *splash_layer = NULL;
    K_TRY(layer_create(screen, 999, &splash_layer));

    layer_draw_begin(splash_layer);

    canvas_clear(&splash_layer->canvas, MAKE_COLOR(12, 15, 20));

    uint64_t scale_title = 10;
    uint64_t scale_ver = 6;

    size_t len_title = 6; // "NanoOS"
    size_t len_ver = strlen(version_str);

    int64_t title_width = (int64_t)(len_title * 6 * scale_title);
    int64_t ver_width = (int64_t)(len_ver * 6 * scale_ver);

    int64_t center_x = (int64_t)(screen.width / 2);
    int64_t center_y = (int64_t)(screen.height / 2);

    int64_t title_x = center_x - (title_width / 2);
    int64_t title_y = center_y - 50;

    int64_t ver_x = center_x - (ver_width / 2);
    int64_t ver_y = title_y + (int64_t)(5 * scale_title) + 15;

    draw_string(&splash_layer->canvas, title_x, title_y, "NanoOS", COLOR_WHITE, scale_title);
    draw_string(&splash_layer->canvas, ver_x, ver_y, version_str, COLOR_CYAN, scale_ver);

    layer_draw_end(splash_layer);

    thread_sleep_ms(duration_ms);

    layer_destroy(splash_layer);

    return E_OK;
}
