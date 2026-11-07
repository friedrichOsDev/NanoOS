/**
 * @file gfx.c
 * @brief Video driver HAL
 * @author friedrichOsDev
 */

#include <arch/multiboot2.h>
#include <drivers/gfx/compositor.h>
#include <drivers/gfx/fb_driver.h>
#include <drivers/gfx/gfx.h>
#include <mm/memory.h>
#include <pch.h>

static video_mode_t current_video_mode = {0};
static video_driver_t *driver_list = NULL;
static video_driver_t *current_driver = NULL;

/**
 * @brief Initializes the video driver HAL and maps the Multiboot2 framebuffer into virtual memory.
 *
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t gfx_init() {
    if (kernel_fb_info.fb_width == 0 || kernel_fb_info.fb_height == 0) {
        log_warn("gfx: no valid multiboot2 framebuffer available");
        return E_NOTAVAIL;
    }

    current_video_mode.width = kernel_fb_info.fb_width;
    current_video_mode.height = kernel_fb_info.fb_height;
    current_video_mode.bpp = kernel_fb_info.fb_bpp;
    current_video_mode.pitch = kernel_fb_info.fb_pitch;
    current_video_mode.framebuffer_phys = kernel_fb_info.fb_addr;
    current_video_mode.framebuffer_size = (size_t)(kernel_fb_info.fb_pitch * kernel_fb_info.fb_height);
    current_video_mode.fb_type = (framebuffer_type_t)kernel_fb_info.fb_type;
    current_video_mode.flags = FB_FLAG_DOUBLE_BUFFERED;

    current_video_mode.red_mask_size = kernel_fb_info.red_mask_size;
    current_video_mode.red_field_position = kernel_fb_info.red_field_position;
    current_video_mode.green_mask_size = kernel_fb_info.green_mask_size;
    current_video_mode.green_field_position = kernel_fb_info.green_field_position;
    current_video_mode.blue_mask_size = kernel_fb_info.blue_mask_size;
    current_video_mode.blue_field_position = kernel_fb_info.blue_field_position;

    K_TRY(vmm_map_mmio(vmm_kernel_space(), current_video_mode.framebuffer_phys, current_video_mode.framebuffer_size, &current_video_mode.framebuffer));

    log_debug("initialized video mode %lux%lu @ %lu bpp (Pitch: %lu)", current_video_mode.width, current_video_mode.height, current_video_mode.bpp, current_video_mode.pitch);

    K_TRY(fb_driver_init());
    K_TRY(compositor_init());
    compositor_enable_rendering();

    return E_OK;
}

/**
 * @brief Deinitializes the video HAL subsystem.
 *
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t gfx_deinit() {
    if (current_driver && current_driver->deinit) {
        current_driver->deinit();
    }
    return E_OK;
}

/**
 * @brief Sets the global video mode across active drivers.
 *
 * @param[in] mode Pointer to the target video mode descriptor.
 * @return E_OK on success, or an error code if the driver rejects the mode.
 */
kerr_t gfx_set_mode(video_mode_t *mode) {
    if (!mode) {
        return E_INVAL;
    }

    if (current_driver && current_driver->set_mode) {
        K_TRY(current_driver->set_mode(mode));
    }

    current_video_mode = *mode;
    return E_OK;
}

/**
 * @brief Retrieves the current active video mode descriptor.
 *
 * @param[out] out Double pointer to store the video mode descriptor pointer.
 * @return E_OK on success, or E_INVAL if out is NULL.
 */
kerr_t gfx_get_current_mode(video_mode_t **out) {
    if (!out) {
        return E_INVAL;
    }
    *out = &current_video_mode;
    return E_OK;
}

/**
 * @brief Registers a new video driver in the HAL.
 *
 * @param[in] driver Pointer to the video driver structure.
 * @return E_OK on success, or E_INVAL if driver is NULL.
 */
kerr_t gfx_register_driver(video_driver_t *driver) {
    if (!driver) {
        return E_INVAL;
    }

    driver->next = driver_list;
    driver_list = driver;

    if (!current_driver) {
        K_TRY(gfx_set_current_driver(driver));
    }

    log_debug("registered driver: %s", driver->driver_name ? driver->driver_name : "unnamed");
    return E_OK;
}

/**
 * @brief Switches the currently active video driver.
 *
 * Deinitializes and disables the old driver (if present), assigns the new driver,
 * initializes and enables it, and applies the current video mode.
 *
 * @param[in] driver Pointer to the new video driver.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t gfx_set_current_driver(video_driver_t *driver) {
    if (!driver) {
        return E_INVAL;
    }

    if (current_driver) {
        if (current_driver->disable) {
            current_driver->disable();
        }
        if (current_driver->deinit) {
            current_driver->deinit();
        }
    }

    current_driver = driver;

    if (current_driver->init) {
        K_TRY(current_driver->init());
    }
    if (current_driver->enable) {
        current_driver->enable();
    }

    if (current_driver->set_mode) {
        K_TRY(current_driver->set_mode(&current_video_mode));
    }

    log_debug("switched active driver to: %s", driver->driver_name ? driver->driver_name : "unnamed");
    return E_OK;
}

/**
 * @brief Flushes framebuffer content through the current active driver.
 *
 * @param[in] data Pointer to framebuffer data to flush.
 * @param[in] size Size in bytes of data to flush.
 */
void gfx_flush(const void *data, size_t size) {
    if (current_driver && current_driver->flush) {
        current_driver->flush(data, size);
    }
}

/**
 * @brief Retrieves the head of the registered video drivers list.
 *
 * @return Pointer to the first registered driver.
 */
video_driver_t *gfx_get_driver_list() {
    return driver_list;
}

/**
 * @brief Retrieves the currently active video driver.
 *
 * @return Pointer to the active driver, or NULL if none.
 */
video_driver_t *gfx_get_current_driver() {
    return current_driver;
}
