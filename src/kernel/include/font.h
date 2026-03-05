/**
 * @file font.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <fb.h>

/**
 * @brief Represents the header of a PSF2 font file.
 */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t numglyph;
    uint32_t bytesperglyph;
    uint32_t height;
    uint32_t width;
} __attribute__((packed)) psf2_header_t;

/**
 * @brief Structure to hold foreground and background colors for text rendering.
 */
typedef struct {
    color_t fg_color;
    color_t bg_color;
} font_color_t;

void font_init(void);
uint32_t font_get_width(void);
uint32_t font_get_height(void);
void* font_get_glyph(uint32_t unicode);
