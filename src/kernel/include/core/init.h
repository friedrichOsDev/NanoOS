/**
 * @file init.h
 * @brief Schnittstellen und Datenstrukturen für die Kernel-Initialisierung.
 * @details Definiert Symbole des Kernel-Linkers, Typen zur Multiboot2-Speicherkarte,
 *          Framebuffer-Informationen sowie Boot-Module.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/**
 * @brief Haupt-Einstiegspunkt des Kernels nach dem Bootloader-Übergang.
 * @param magic Multiboot2 Magic Number (muss 0x36d76289 entsprechen).
 * @param info_ptr Physikalische Adresse der Multiboot2 Informationsstruktur.
 */
void kernel_init(uint64_t magic, uint64_t info_ptr);
