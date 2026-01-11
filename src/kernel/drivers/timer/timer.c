#include <timer.h>
#include <handler.h>
#include <io.h>
#include <serial.h>

uint32_t ticks = 0;

void timer_callback(uint32_t irq) {
    (void)irq;
    ticks++;
}

uint32_t timer_get_ticks(void) {
    return ticks;
}

void timer_init(uint32_t frequency) {
    irq_install_handler(32, timer_callback);
    serial_puts("timer_init: installed timer callback handler\n");
    uint32_t divisor = 1193180 / frequency;
    
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    serial_puts("timer_init: frequency set to ");
    serial_put_int(frequency);
    serial_puts(" Hz\n");
    
}