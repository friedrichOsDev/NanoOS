/**
 * @file tss.c
 * @brief 64-bit Task State Segment Setup
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/tss.h>
#include <arch/x86_64/drivers/serial.h>

struct tss_entry tss;

extern uint8_t stack_top[];
static uint8_t double_fault_stack[4096];

/**
 * Initializes the TSS
 */
void tss_init() {
    serial_printf(COM1, "TSS: initializing\n");

    for (uint32_t i = 0; i < sizeof(struct tss_entry); i++) {
        ((uint8_t *)&tss)[i] = 0;
    }

    tss.rsp0 = (uint64_t)stack_top; // set Ring 0 Stack
    tss.ist1 = (uint64_t)&double_fault_stack[sizeof(
        double_fault_stack)]; // set Interrupt Stack Table 1 (IST1) for Double
                              // Faults
    tss.iomap_base = sizeof(struct tss_entry);

    serial_printf(COM1, "TSS: init done (rsp0 = %p, ist1 = %p)\n",
                  (void *)tss.rsp0, (void *)tss.ist1);
}
