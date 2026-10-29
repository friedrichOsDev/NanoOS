/**
 * @file pci.h
 * @brief PCIe / PCI MMCONFIG Subsystem und Treiber-Framework.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

typedef struct pci_device {
    uint16_t vendor_id;  /**< Hersteller-ID (Vendor ID) */
    uint16_t device_id;  /**< Geräte-ID (Device ID) */
    uint8_t class_code;  /**< Gerätekategorie (Class Code) */
    uint8_t subclass;    /**< Unterkategorie (Subclass Code) */
    uint8_t prog_if;     /**< Programmierschnittstelle (Programming Interface) */
    uint8_t revision_id; /**< Revisionsnummer */

    uint8_t bus;      /**< PCI-Bus-Nummer */
    uint8_t device;   /**< PCI-Gerätenummer */
    uint8_t function; /**< PCI-Funktionsnummer */

    virt_addr_t config_space_virt; /**< Virtuelle Basisadresse des ECAM-Konfigurationsraums */

    uint64_t bar[6];      /**< Basisadressregister (BAR 0-5) */
    uint64_t bar_size[6]; /**< Speichergröße der jeweiligen BARs */
    bool is_mmio_bar[6];  /**< true wenn MMIO-BAR, false wenn I/O-Port-BAR */
    bool is_64bit_bar[6]; /**< true wenn 64-Bit-Adressierung verwendet wird */

    uint8_t msi_cap_offset;  /**< Offset der MSI Capability im Konfigurationsraum (0 falls nicht vorhanden) */
    uint8_t msix_cap_offset; /**< Offset der MSI-X Capability im Konfigurationsraum (0 falls nicht vorhanden) */

    struct pci_device *next; /**< Zeiger auf das nächste Gerät in der verketteten Liste */
} pci_device_t;

typedef bool (*pci_driver_probe_t)(pci_device_t *dev);

typedef struct pci_driver {
    const char *name;         /**< Name des Treibers */
    uint16_t vendor_id;       /**< Ziel-Hersteller-ID (0xFFFF für Beliebig/Wildcard) */
    uint16_t device_id;       /**< Ziel-Geräte-ID (0xFFFF für Beliebig/Wildcard) */
    uint8_t class_code;       /**< Ziel-Geräteklasse (0xFF für Beliebig/Wildcard) */
    uint8_t subclass;         /**< Ziel-Unterklasse (0xFF für Beliebig/Wildcard) */
    pci_driver_probe_t probe; /**< Callback-Funktion zur Initialisierung des Geräts */
    struct pci_driver *next;  /**< Zeiger auf den nächsten Treiber in der Liste */
} pci_driver_t;

void pci_init();
void pci_register_driver(pci_driver_t *driver);
