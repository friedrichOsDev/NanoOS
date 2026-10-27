/**
 * @file panic.c
 * @brief Implementierung der Kernel-Panic-Routine zur Behandlung kritischer Systemfehler.
 * @details Deaktiviert die lokale Interruptverarbeitung, signalisiert allen weiteren
 *          Prozessoren das sofortige Anhalten und gibt Diagnoseinformationen
 *          über die serielle Schnittstelle aus.
 * @author friedrichOsDev
 */

#include <arch/arch.h>
#include <arch/x86_64/cpu/apic.h>
#include <core/panic.h>

void panic(const char *message, uint64_t error_code) {
    arch_cli(); // idt disable (turn off interrupts)
    lapic_send_broadcast_stop_ipi();
    serial_printf("KERNEL PANIC: %s (Error code %llx)\n", message, error_code);
    while (1)
        arch_hlt();
}
