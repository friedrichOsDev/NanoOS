#include "include/kernel.h"
#include "include/fb.h"

void kernel_main();

// Force this function to be in a special section called ".text.entry"
void __attribute__((section(".text.entry"))) _start() {
    kernel_main();
    while(1);
}

void kernel_main() {
    fb_init();
    fb_draw_rect(50, 50, 100, 100, 0xFFFFFF);
    
    while(1);
}