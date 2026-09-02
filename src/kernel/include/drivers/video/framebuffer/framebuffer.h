/**
 * @file framebuffer.h
 * @brief Simple Framebuffer Driver
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t *backbuffer;
    size_t backbuffer_size;
    uint64_t scroll_offset;
} backbuffer_info_t;

typedef struct {
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

extern color_t black;
extern color_t white;
extern color_t red;
extern color_t green;
extern color_t blue;

void framebuffer_init_thread();