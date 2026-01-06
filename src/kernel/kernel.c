#include "include/kernel.h"

void kernel_main() {
    // draw a blue rect using the info (framebuffer) from bootloader
    volatile uint32_t* framebuffer = (uint32_t*)(mode_info->framebuffer);
    
    // Safety check: If VBE failed or address is 0, do not draw
    if (mode_info->framebuffer == 0) return;

    // Eine Variable für die Farbe (Blau)
    uint32_t color = 0x0000FF;

    for (uint32_t y = 100; y < 200; y++) {
        for (uint32_t x = 100; x < 200; x++) {
            framebuffer[y * mode_info->pitch / 4 + x] = color; 
        }
    }

    // Infinite loop to prevent return
    while(1) {}
}