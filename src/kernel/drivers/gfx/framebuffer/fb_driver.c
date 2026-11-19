/**
 * @file fb_driver.c
 * @brief Framebuffer video driver
 * @author friedrichOsDev
 */

#include <drivers/gfx/fb_driver.h>
#include <drivers/gfx/gfx.h>
#include <lib/util/mem.h>
#include <pch.h>

static bool rendering = false;
static video_mode_t *current_mode = NULL;

/** @brief Enables the framebuffer driver output. */
static void fb_enable() {
    log_debug("framebuffer driver enabled");
    rendering = true;
}

/** @brief Disables the framebuffer driver output and clears the screen. */
static void fb_disable() {
    log_debug("framebuffer driver disabled");
    rendering = false;
    if (!current_mode || !current_mode->framebuffer) {
        return;
    }
    void *dest = (void *)current_mode->framebuffer;
    memset(dest, 0x00, current_mode->framebuffer_size);
}

/**
 * @brief Initializes the framebuffer driver and queries the active video mode.
 *
 * @return E_OK on success, or an appropriate error code on failure.
 */
static kerr_t fb_init() {
    K_TRY(gfx_get_current_mode(&current_mode));
    log_debug("framebuffer driver initialized");
    return E_OK;
}

/** @brief Deinitializes the framebuffer driver. */
static void fb_deinit() {
    rendering = false;
    log_debug("framebuffer driver deinitialized");
}

/**
 * @brief Sets a specific video mode (if supported by the framebuffer hardware).
 *
 * @param[in] mode Pointer to the desired video mode descriptor.
 * @return E_OK on success, or E_NOTAVAIL if dynamic mode switching is unsupported.
 */
static kerr_t fb_set_mode(video_mode_t *mode) {
    if (!mode) {
        return E_INVAL;
    }

    if (current_mode &&
        mode->width == current_mode->width &&
        mode->height == current_mode->height &&
        mode->bpp == current_mode->bpp) {
        return E_OK;
    }

    log_warn("dynamic mode switching not supported by basic framebuffer");
    return E_NOTAVAIL;
}

/**
 * @brief Retrieves the active video mode descriptor from the driver.
 *
 * @param[out] out Double pointer to store the video mode descriptor pointer.
 * @return E_OK on success, or E_INVAL if out is NULL.
 */
static kerr_t fb_get_mode(video_mode_t **out) {
    if (!out) {
        return E_INVAL;
    }
    *out = current_mode;
    return E_OK;
}

/**
 * @brief Flushes framebuffer pixel data by copying to the physical framebuffer memory.
 *
 * @param[in] data Pointer to source framebuffer pixel data.
 * @param[in] size Size in bytes of data to flush.
 */
static void fb_flush(const void *data, size_t size) {
    if (!data || size == 0 || !current_mode || !current_mode->framebuffer || !rendering) {
        return;
    }

    size_t copy_size = (size > current_mode->framebuffer_size) ? current_mode->framebuffer_size : size;
    volatile uint32_t *dest = (volatile uint32_t *)current_mode->framebuffer;
    const uint32_t *src = (const uint32_t *)data;
    size_t dwords = copy_size / sizeof(uint32_t);

    for (size_t i = 0; i < dwords; i++) {
        dest[i] = src[i];
    }
}

/**
 * @brief Flushes a partial rectangular region of the framebuffer to physical memory.
 *
 * @param[in] data Pointer to source framebuffer pixel data.
 * @param[in] pitch Scanline pitch in bytes of source data.
 * @param[in] x Top-left X coordinate.
 * @param[in] y Top-left Y coordinate.
 * @param[in] width Width in pixels to flush.
 * @param[in] height Height in pixels to flush.
 */
static void fb_flush_rect(const void *data, size_t pitch, uint64_t x, uint64_t y, uint64_t width, uint64_t height) {
    if (!data || !current_mode || !current_mode->framebuffer || !rendering) {
        return;
    }

    if (x >= current_mode->width || y >= current_mode->height || width == 0 || height == 0) {
        return;
    }

    if (x + width > current_mode->width) {
        width = current_mode->width - x;
    }
    if (y + height > current_mode->height) {
        height = current_mode->height - y;
    }

    const uint8_t *src_bytes = (const uint8_t *)data;
    uint8_t *dest_bytes = (uint8_t *)current_mode->framebuffer;

    for (uint64_t row = 0; row < height; row++) {
        uint64_t cur_y = y + row;
        size_t dest_offset = (size_t)(cur_y * current_mode->pitch + x * sizeof(uint32_t));
        size_t src_offset = (size_t)(cur_y * pitch + x * sizeof(uint32_t));

        volatile uint32_t *dest_row = (volatile uint32_t *)(void *)(dest_bytes + dest_offset);
        const uint32_t *src_row = (const uint32_t *)(const void *)(src_bytes + src_offset);

        for (size_t col = 0; col < width; col++) {
            dest_row[col] = src_row[col];
        }
    }
}

/** @brief Static definition of the standard linear framebuffer driver. */
static video_driver_t fb_driver = {
    .driver_name = "framebuffer-driver",
    .enable = fb_enable,
    .disable = fb_disable,
    .init = fb_init,
    .deinit = fb_deinit,
    .set_mode = fb_set_mode,
    .get_mode = fb_get_mode,
    .flush = fb_flush,
    .flush_rect = fb_flush_rect,
    .next = NULL};

/**
 * @brief Registers and initializes the linear framebuffer driver in the video HAL.
 *
 * @return E_OK on success, or an error code on failure.
 */
kerr_t fb_driver_init() {
    K_TRY(gfx_register_driver(&fb_driver));
    log_debug("framebuffer driver registered successfully");
    return E_OK;
}
