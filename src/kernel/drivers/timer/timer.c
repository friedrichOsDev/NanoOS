/*
 * @file timer.c
 * @brief Timer drive implementation
 * @author friedrichOsDev
 */

#include <timer.h>
#include <handler.h>
#include <io.h>
#include <serial.h>

/*
 * Keeps track of the number of timer ticks since system start
 */
uint32_t ticks = 0;

/*
 * Timer IRQ callback
 * @param irq The IRQ number (should be 32 for timer)
 */
void timer_callback(uint32_t irq) {
    (void)irq;
    ticks++;
}

/*
 * Get the number of timer ticks since system start
 * @param void
 * @return Number of ticks
 */
uint32_t timer_get_ticks(void) {
    return ticks;
}

/*
 * A function to initialize the timer driver with a specified frequency
 * @param frequency The desired frequency in Hz
 */
void timer_init(uint32_t frequency) {
    irq_install_handler(32, timer_callback);
    uint32_t divisor = 1193180 / frequency;
    
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}