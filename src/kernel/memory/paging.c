#include <paging.h>
#include <serial.h>

extern void load_page_directory(uint32_t* directory);
extern void enable_paging();

void paging_init() {
    serial_puts("paging_init: paging initialization started\n");
}