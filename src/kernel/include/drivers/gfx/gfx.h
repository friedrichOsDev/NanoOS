/**
 * @file gfx.h
 * @brief Video driver HAL
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/** @brief Framebuffer memory model types. */
typedef enum {
    FB_TYPE_INDEXED = 0, /**< Palette-based indexed color */
    FB_TYPE_RGB = 1,     /**< Direct RGB color bitfields */
    FB_TYPE_TEXT = 2     /**< EGA text mode */
} framebuffer_type_t;

/** @brief Framebuffer operational flags. */
typedef enum {
    FB_FLAG_NONE = 0,
    FB_FLAG_DOUBLE_BUFFERED = 1 << 0, /**< Double buffering enabled */
} framebuffer_flags_t;

/** @brief Comprehensive video mode descriptor. */
typedef struct {
    uint64_t width;               /**< Screen width in pixels */
    uint64_t height;              /**< Screen height in pixels */
    uint64_t bpp;                 /**< Bits per pixel */
    uint64_t pitch;               /**< Bytes per scanline */
    uint64_t refresh_rate;        /**< Refresh rate in Hz (if available) */
    virt_addr_t framebuffer;      /**< Virtual address of the framebuffer */
    phys_addr_t framebuffer_phys; /**< Physical address of the framebuffer */
    size_t framebuffer_size;      /**< Total size in bytes */
    framebuffer_type_t fb_type;   /**< Framebuffer memory model type */
    framebuffer_flags_t flags;    /**< Operational flags */
    uint8_t red_mask_size;        /**< Red bit width */
    uint8_t red_field_position;   /**< Red bit offset */
    uint8_t green_mask_size;      /**< Green bit width */
    uint8_t green_field_position; /**< Green bit offset */
    uint8_t blue_mask_size;       /**< Blue bit width */
    uint8_t blue_field_position;  /**< Blue bit offset */
} video_mode_t;

/** @brief Video driver interface structure (VTable). */
typedef struct video_driver {
    const char *driver_name;                      /**< Name of the video driver */
    void (*enable)();                             /**< Enable driver / output */
    void (*disable)();                            /**< Disable driver / output */
    kerr_t (*init)();                             /**< Initialize driver */
    void (*deinit)();                             /**< Deinitialize driver */
    kerr_t (*set_mode)(video_mode_t *mode);       /**< Set a specific video mode */
    kerr_t (*get_mode)(video_mode_t **out);       /**< Get driver's active video mode */
    void (*flush)(const void *data, size_t size); /**< Flush framebuffer changes (if needed) */
    struct video_driver *next;                    /**< Pointer to the next registered driver */
} video_driver_t;

kerr_t gfx_init();
kerr_t gfx_deinit();
kerr_t gfx_set_mode(video_mode_t *mode);
kerr_t gfx_get_current_mode(video_mode_t **out);
kerr_t gfx_register_driver(video_driver_t *driver);
kerr_t gfx_set_current_driver(video_driver_t *driver);
video_driver_t *gfx_get_driver_list();
video_driver_t *gfx_get_current_driver();
void gfx_flush(const void *data, size_t size);
