/**
 * @file panic.c
 * @brief Implementierung der Kernel-Panic-Routine zur Behandlung kritischer Systemfehler.
 * @details Deaktiviert die lokale Interruptverarbeitung, signalisiert allen weiteren
 *          Prozessoren das sofortige Anhalten und gibt Diagnoseinformationen
 *          über die serielle Schnittstelle aus.
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/apic.h>
#include <arch/x86_64/cpu/interrupts.h>
#include <arch/x86_64/drivers/serial.h>
#include <core/panic.h>

void panic(const char *message, uint64_t error_code) {
    idt_disable();
    lapic_send_broadcast_stop_ipi();
    serial_printf(COM1, "KERNEL PANIC: %s (Error code %llx)\n", message, error_code);
    while (1)
        __asm__ __volatile__("hlt");
}