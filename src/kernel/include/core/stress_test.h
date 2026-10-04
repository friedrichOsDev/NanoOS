/**
 * @file stress_test.h
 * @brief NanoOS Integration & System-Stresstest-Suite.
 * @author friedrichOsDev
 */

#pragma once

/**
 * @brief Führt die gesamte NanoOS Integration- und Stresstest-Suite aus.
 *
 * Diese Funktion durchläuft alle Testphasen (PMM/VMM/Heap, ACPI/APIC/HPET,
 * Scheduler/FPU, SMP/IPIs, Exception-Handling, MMIO sowie Thread-Sleep-Präzision)
 * und gibt die Testergebnisse sequentiell über die serielle Schnittstelle (COM1) aus.
 */
void run_kernel_stress_test();