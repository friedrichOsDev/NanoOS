/**
 * @file smp_trampoline.h
 * @brief Deklarationen für den SMP Trampoline Boot Code (AP-Startups).
 * @author friedrichOsDev
 */

#include <stdint.h>

/**
 * @brief Startadresse des Trampoline-Codes im Speicher.
 *
 * Zeigt auf den Beginn des 16-Bit Real Mode Codes, der an eine niedrige
 * Adresse (z. B. 0x8000) im RAM kopiert wird.
 */
extern uint8_t smp_trampoline_start[];

/**
 * @brief Endadresse des Trampoline-Codes im Speicher.
 */
extern uint8_t smp_trampoline_end[];

/**
 * @brief Physikalische Adresse der PML4-Page-Table für den Anwendungsprozessor (AP).
 *
 * Muss vor dem Senden des SIPI (Startup IPI) auf die Basisadresse der Seitentabelle gesetzt werden.
 */
extern uint64_t smp_trampoline_pml4;

/**
 * @brief Kernel-Stack-Pointer für den zu startenden AP.
 *
 * @note Muss zwingend 16-Byte-aligned sein.
 */
extern uint64_t smp_trampoline_stack;

/**
 * @brief 64-Bit Kernel-Einstiegsfunktion für den AP.
 *
 * Zeigt auf die C-Funktion im Kernel, die der AP nach dem Übergang in den 64-Bit Long Mode ausführt.
 */
extern uint64_t smp_trampoline_entry;