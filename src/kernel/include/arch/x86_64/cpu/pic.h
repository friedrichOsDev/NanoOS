/**
 * @file pic.h
 * @brief Schnittstelle zum Deaktivieren des Legacy-8259-PICs für den APIC-Betrieb.
 * @author friedrichOsDev
 */

#pragma once

/** @brief E/A-Port für das Befehlsregister des Master-PICs */
#define PIC1_COMMAND 0x20
/** @brief E/A-Port für das Datenregister (Interrupt-Maske) des Master-PICs */
#define PIC1_DATA 0x21
/** @brief E/A-Port für das Befehlsregister des Slave-PICs */
#define PIC2_COMMAND 0xA0
/** @brief E/A-Port für das Datenregister (Interrupt-Maske) des Slave-PICs */
#define PIC2_DATA 0xA1
/** @brief Initialisierungswert ICW1 (Initialization Command Word 1) */
#define ICW1_INIT 0x11
/** @brief Initialisierungswert ICW4 (8086/8088-Modus) */
#define ICW4_8086 0x01

/**
 * @brief Maskiert alle Spuren/Leitungen des Legacy-PICs, um Konflikte mit APIC-Routing zu vermeiden.
 */
void pic_disable();