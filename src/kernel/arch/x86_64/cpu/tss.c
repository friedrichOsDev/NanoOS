/**
 * @file tss.c
 * @brief 64-bit Task State Segment Setup
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/tss.h>
#include <arch/x86_64/drivers/serial.h>

struct tss_entry tss;

struct tss_entry tss_cores[MAX_CPUS];
static uint8_t double_fault_stacks[MAX_CPUS][4096];

/**
 * Initializes the TSS
 */
void tss_init_core(size_t cpu_id, uint64_t kernel_stack) {
    struct tss_entry *tss = &tss_cores[cpu_id];

    for (size_t i = 0; i < sizeof(struct tss_entry); i++) {
        ((uint8_t *)tss)[i] = 0;
    }

    tss->rsp0 = kernel_stack;
    tss->ist1 =
        (uint64_t)&double_fault_stacks[cpu_id]
                                      [sizeof(double_fault_stacks[cpu_id])];
    tss->iomap_base = sizeof(struct tss_entry);

    serial_printf(COM1, "TSS Core %d: init done (rsp0 = %p, ist1 = %p)\n",
                  (int)cpu_id, (void *)tss->rsp0, (void *)tss->ist1);
}