#include <kernel/panic.h>
#include <pch.h>

#include <arch/arch.h>

void panic() {
    arch_cli(); // idt disable (turn off interrupts)
    arch_ipi_stop_others();
    serial_printf("KERNEL PANIC\n");
    while (1)
        arch_hlt();
}
