/**
 * @file timer.c
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

/**
 * @brief Small delay for I/O operations to ensure hardware synchronization.
 */
static void timer_io_wait(void) {
    outb(0x80, 0);
}

/**
 * @brief IRQ 0 handler called on every PIT tick.
 * 
 * Increments the tick counter and processes registered timer events.
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

/**
 * @brief Initializes the PIT to the frequency defined by TIMER_FREQUENCY.
 * 
 * Sets up the hardware registers and installs the IRQ handler.
 */
void timer_init(void) {
    serial_printf("Timer: start\n");
    serial_printf("Timer: set PIT frequency\n");
    uint32_t divisor = 1193180 / TIMER_FREQUENCY;
    outb(0x43, 0x36); // command byte: channel
    timer_io_wait();
    outb(0x40, divisor & 0xFF); // low byte
    timer_io_wait();
    outb(0x40, (divisor >> 8) & 0xFF); // high byte

    serial_printf("Timer: install IRQ handler\n");
    irq_install_handler(0, timer_callback);
    serial_printf("Timer: done\n");
}

/**
 * @brief Registers a new timed event.
 * 
 * @param event The event structure containing the handler and timing info.
 * @return uint32_t The unique event ID, or 0 if the event could not be added.
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

/**
 * @brief Deactivates a registered event by its ID.
 * 
 * @param event_id The ID returned by timer_add_event.
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

/**
 * @brief Returns the current number of system ticks since boot.
 */
uint32_t timer_get_ticks(void) {
    return ticks;
}

/**
 * @brief Blocks execution for a specified number of milliseconds.
 * 
 * Uses the 'hlt' instruction to yield the CPU while waiting for ticks.
 * 
 * @param ms Number of milliseconds to sleep.
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
