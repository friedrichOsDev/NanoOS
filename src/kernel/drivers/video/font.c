/**
 * @file font.c
 * @author friedrichOsDev
 */

#include <font.h>
#include <serial.h>

extern uint8_t _binary_assets_font_psf_start[];
extern uint8_t _binary_assets_font_psf_end[];

static psf2_header_t *font_header;
static void *glyph_table;
static uint8_t *unicode_table;

/**
 * @brief Initializes the font subsystem.
 */
void font_init(void) {
    font_header = (psf2_header_t*)_binary_assets_font_psf_start;

    serial_printf("FONT: Font data address: %p\n", _binary_assets_font_psf_start);
    serial_printf("FONT: Reading magic: %x\n", font_header->magic);

    if (font_header->magic != 0x864ab572) {
        serial_printf("FONT: Invalid PSF2 magic number!\n");
        return;
    }

    glyph_table = (uint8_t*)_binary_assets_font_psf_start + font_header->headersize;
    unicode_table = (uint8_t*)_binary_assets_font_psf_start + (font_header->headersize + font_header->numglyph * font_header->bytesperglyph);

    serial_printf("FONT: PSF2 font loaded. Version: %d, Glyphs: %d, Size: %dx%d\n", font_header->version, font_header->numglyph, font_header->width, font_header->height);
}

/**
 * @brief Gets the width of a single character glyph.
 * @return The width in pixels.
 */
uint32_t font_get_width(void) {
    return font_header->width;
}

/**
 * @brief Gets the height of a single character glyph.
 * @return The height in pixels.
 */
uint32_t font_get_height(void) {
    return font_header->height;
}

/**
 * @brief Finds the glyph data for a given Unicode character.
 * @param unicode The Unicode character to find.
 * @return A pointer to the start of the glyph's bitmap data, or NULL if not found.
 */
void* font_get_glyph(uint32_t unicode) {
    if (!font_header || font_header->magic != 0x864ab572) {
        return NULL;
    }
    
    uint32_t glyph_index = 0;

    if (font_header->flags & 1) {
        uint8_t* p = unicode_table;
        uint32_t current_glyph = 0;
        while (p < _binary_assets_font_psf_end && current_glyph < font_header->numglyph) {
            uint32_t entry_unicode = 0;
            int len = 0;
            // UTF-8 decoding of the table
            if ((p[0] & 0x80) == 0) { // 1-byte sequence
                entry_unicode = p[0];
                len = 1;
            } else if ((p[0] & 0xE0) == 0xC0) { // 2-byte sequence
                entry_unicode = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
                len = 2;
            } else if ((p[0] & 0xF0) == 0xE0) { // 3-byte sequence
                entry_unicode = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
                len = 3;
            } else if ((p[0] & 0xF8) == 0xF0) { // 4-byte sequence
                entry_unicode = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
                len = 4;
            } else if (p[0] == 0xFF) { // Sequence terminator
                p++;
                current_glyph++;
                continue;
            } else { // Invalid sequence
                p++; 
                continue;
            }
            
            if (entry_unicode == unicode) {
                glyph_index = current_glyph;
                break;
            }

            p += len;
        }
        if (p >= _binary_assets_font_psf_end) {
             glyph_index = unicode < font_header->numglyph ? unicode : 0;
        }

    } else {
        glyph_index = (unicode < font_header->numglyph) ? unicode : 0;
    }
    
    if(glyph_index == 0 && unicode != 0 && unicode != 0xFFFD) {
        return font_get_glyph(0xFFFD);
    }

    return (uint8_t*)glyph_table + glyph_index * font_header->bytesperglyph;
}
