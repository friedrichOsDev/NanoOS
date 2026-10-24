/**
 * @file init.h
 * @brief Schnittstellen und Datenstrukturen für die Kernel-Initialisierung.
 * @details Definiert Symbole des Kernel-Linkers, Typen zur Multiboot2-Speicherkarte,
 *          Framebuffer-Informationen sowie Boot-Module.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

#define KERNEL_START (uintptr_t) kernel_start
#define KERNEL_END (uintptr_t) kernel_end
#define KERNEL_START_PHYS (uintptr_t) kernel_start_phys
#define KERNEL_END_PHYS (uintptr_t) kernel_end_phys

/**
 * @brief Haupt-Einstiegspunkt des Kernels nach dem Bootloader-Übergang.
 * @param magic Multiboot2 Magic Number (muss 0x36d76289 entsprechen).
 * @param info_ptr Physikalische Adresse der Multiboot2 Informationsstruktur.
 */
void kernel_init(uint64_t magic, uint64_t info_ptr);
