#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

void serial_init();
void __attribute__((__target__("no-sse"))) serial_putc(char c);
void __attribute__((__target__("no-sse"))) serial_puts(const char* s);
void __attribute__((__target__("no-sse"))) serial_put_hex(uint32_t value);
void __attribute__((__target__("no-sse"))) serial_put_int(uint32_t value);

#endif // SERIAL_H
