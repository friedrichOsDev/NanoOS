/**
 * @file font.h
 * @brief Font definitions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

extern const uint8_t nanoos_font_start[];
extern const uint8_t nanoos_font_end[];

/** @brief PSF2 font structure */
typedef struct {
    uint32_t header_size;         /**< Size of the PSF2 header in bytes */
    uint32_t flags;               /**< Font flags */
    uint32_t glyph_count;         /**< Total number of glyphs contained in the font */
    uint32_t glyph_size;          /**< Size of a single glyph in bytes */
    uint32_t height;              /**< Height of each glyph in pixels */
    uint32_t width;               /**< Width of each glyph in pixels */
    const uint8_t *glyph_data;    /**< Pointer to the raw glyph bitmap data */
    const uint8_t *unicode_table; /**< Pointer to the optional Unicode mapping table */
    const uint8_t *end;           /**< Pointer to the end of the font memory buffer for bounds checking */
} psf2_font_t;
