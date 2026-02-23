/*
 * @file timer.c
 * @brief Timer driver
 * @author friedrichOsDev
 */

#include <timer.h>
#include <io.h>
#include <handler.h>
#include <serial.h>
#include <panic.h>

static volatile uint32_t ticks = 0;
static event_t events[TIMER_MAX_EVENTS];
static uint32_t event_count = 0;
static uint32_t next_id = 0;

/*
 * Timer interrupt callback function
 * @param regs Pointer to the registers structure containing the CPU state at the time of the interrupt
 */
void timer_callback(struct registers *regs) {
    (void)regs;
    
    // event handling
    for (uint32_t i = 0; i < event_count; i++) {
        if (!events[i].active) continue;
        if (events[i].target_tick <= ticks) {
            events[i].handler();
            if (events[i].repeat) {
                events[i].target_tick += events[i].interval;
            } else {
                events[i].active = false;
            }
        }
    }

    if (ticks == UINT32_MAX) kernel_panic("Timer tick overflow -> system reboot required", ticks);
    ticks++;
}

/*
 * Initialize the timer driver
 * @param void
 */
void timer_init(void) {
    // set frequency
    uint32_t divisor = 1193180 / TIMER_FREQUENCY;
    outb(0x43, 0x36); // command byte: channel
    outb(0x40, divisor & 0xFF); // low byte
    outb(0x40, (divisor >> 8) & 0xFF); // high byte

    irq_install_handler(0, timer_callback);
}

/*
 * Add a timer event to the event list
 * @note do NOT use this function for adding complex events because that slows down the timer interrupt handler. Instead, use this function to add simple events that set flags or similar and then check those flags in the main loop to execute more complex code.
 * @param event The timer event to add
 * @return The ID of the added timer event, or 0 if the event could not be added
 */
uint32_t timer_add_event(event_t event) {
    __asm__ __volatile__("cli");
    uint32_t id = ++next_id;
    event.event_id = id;
    event.active = true;
    for (uint32_t i = 0; i < event_count; i++) {
        if (!events[i].active) {
            events[i] = event;
            __asm__ __volatile__("sti");
            return id;
        }
    }

    if (event_count < TIMER_MAX_EVENTS) {
        events[event_count++] = event;
        __asm__ __volatile__("sti");
        return id;
    } 
        
    
    serial_printf("Timer: Error: Maximum number of timer events reached\n");
    __asm__ __volatile__("sti");
    return 0;
}

/*
 * Remove a timer event from the event list
 * @param event_id The ID of the timer event to remove
 */
void timer_remove_event(uint32_t event_id) {
    __asm__ __volatile__("cli");
    for (uint32_t i = 0; i < event_count; i++) {
        if (events[i].event_id == event_id) {
            events[i].active = false;
        }
    }
    __asm__ __volatile__("sti");
}

/*
 * Get the current number of timer ticks since the system started
 * @param void
 * @return The current number of timer ticks
 */
uint32_t timer_get_ticks(void) {
    return ticks;
}

/*
 * Sleep for a specified number of milliseconds
 * @param ms The number of milliseconds to sleep
 */
void sleep_ms(uint32_t ms) {
    uint32_t seconds = ms / 1000;
    uint32_t remainder = ms % 1000;
    uint32_t added_ticks = seconds * TIMER_FREQUENCY + (remainder * TIMER_FREQUENCY) / 1000;
    uint64_t overflow_check = (uint64_t)ticks + added_ticks;
    uint32_t target_ticks;
    if (overflow_check > UINT32_MAX) target_ticks = UINT32_MAX;
    else target_ticks = (uint32_t)overflow_check;
    while (ticks < target_ticks) __asm__ __volatile__("hlt");
}
