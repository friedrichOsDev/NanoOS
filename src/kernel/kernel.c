#include "include/kernel.h"

void kernel_main() {
    // Read mmap entry count (0x8200 - 0x8202) mmap first entry (0x8204) and video info (0x9000)
    get_bootloader_info();

    // Infinite loop to prevent return
    while(1) {}
}

void get_bootloader_info() {
    // Todo
}
