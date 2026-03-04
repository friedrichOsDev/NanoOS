/**
 * @file font.h
 * @author Daniel Hepper <daniel@hepper.net>
 */

#pragma once

#include <stdint.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 8

/**
 * @brief Basic 8x8 font data for the first 128 ASCII characters.
 */
extern uint8_t font8x8_basic[128][8];
