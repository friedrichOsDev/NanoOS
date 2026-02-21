/*
 * @file font.h
 * @brief Header file for 8x8 monochrome bitmap fonts
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 8

/*
 * Font data for basic ASCII characters (U+0000 to U+007F)
 */
extern uint8_t font8x8_basic[128][8];
