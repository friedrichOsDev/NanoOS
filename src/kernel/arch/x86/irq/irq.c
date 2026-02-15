/*
 * @file irq.c
 * @brief Interrupt Request (IRQ)
 * @author friedrichOsDev
 */

#include <irq.h>
#include <io.h>
#include <pic.h>
#include <idt.h>
#include <serial.h>

/*
 * External assembly IRQ handlers
 * @param void
 */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/*
 * Initialize IRQs by remapping the PIC and setting up IDT entries for each IRQ
 * @param void
 */
void irq_init(void) {
    serial_printf("IRQ: start\n");

    pic_remap();

    void *irq_table[] = {
        irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7,
        irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15
    };

    for (int i = 0; i < 16; i++) {
        idt_set_gate(32 + i, (uint32_t)irq_table[i], 0x08, 0x8E);
        serial_printf("IRQ: %d set\n", i);
    }

    serial_printf("IRQ: done\n");
}