/**
 * @file pci.h
 * @brief PCIe / PCI MMCONFIG Subsystem und Treiber-Framework.
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/cpu/acpi.h>
#include <arch/x86_64/mm/memdef.h>
#include <stdbool.h>
#include <stdint.h>

/** @brief PCI-Geräteklasse: Unklassifiziert */
#define PCI_CLASS_UNCLASSIFIED 0x00
/** @brief PCI-Geräteklasse: Speicherkontroller (Mass Storage) */
#define PCI_CLASS_STORAGE 0x01
/** @brief PCI-Geräteklasse: Netzwerkcontroller */
#define PCI_CLASS_NETWORK 0x02
/** @brief PCI-Geräteklasse: Anzeige/Grafikcontroller */
#define PCI_CLASS_DISPLAY 0x03
/** @brief PCI-Geräteklasse: Multimediacontroller */
#define PCI_CLASS_MULTIMEDIA 0x04
/** @brief PCI-Geräteklasse: Speichercontroller */
#define PCI_CLASS_MEMORY 0x05
/** @brief PCI-Geräteklasse: Brücken-Gerät (Bridge) */
#define PCI_CLASS_BRIDGE 0x06
/** @brief PCI-Geräteklasse: Einfache Kommunikations-Controller */
#define PCI_CLASS_SIMPLE_COMMUNICATION 0x07
/** @brief PCI-Geräteklasse: Basis-Systemperipherie */
#define PCI_CLASS_BASE_SYSTEM_PERIPHERAL 0x08
/** @brief PCI-Geräteklasse: Eingabegeräte */
#define PCI_CLASS_INPUT_DEVICE 0x09
/** @brief PCI-Geräteklasse: Dockingstations */
#define PCI_CLASS_DOCKING_STATION 0x0A
/** @brief PCI-Geräteklasse: Prozessoren */
#define PCI_CLASS_PROCESSOR 0x0B
/** @brief PCI-Geräteklasse: Serielle Bus-Controller */
#define PCI_CLASS_SERIAL_BUS 0x0C
/** @brief PCI-Geräteklasse: Drahtlose Controller */
#define PCI_CLASS_WIRELESS 0x0D
/** @brief PCI-Geräteklasse: Intelligente I/O-Controller */
#define PCI_CLASS_INTELLIGENT 0x0E
/** @brief PCI-Geräteklasse: Satellitenkommunikation */
#define PCI_CLASS_SATELLITE_COMMUNICATION 0x0F
/** @brief PCI-Geräteklasse: Verschlüsselung/Entschlüsselung */
#define PCI_CLASS_ENCRYPTION 0x10
/** @brief PCI-Geräteklasse: Signalverarbeitung */
#define PCI_CLASS_SIGNAL_PROCESSING 0x11
/** @brief PCI-Geräteklasse: Verarbeitungsbeschleuniger */
#define PCI_CLASS_PROCESSING_ACCELERATOR 0x12
/** @brief PCI-Geräteklasse: Nicht-essentielle Messtechnik */
#define PCI_CLASS_NON_ESSENTIAL_INSTRUMENTATION 0x13
/** @brief PCI-Geräteklasse: Co-Prozessor */
#define PCI_CLASS_CO_PROCESSOR 0x40
/** @brief PCI-Geräteklasse: Nicht zugewiesen / Undefiniert */
#define PCI_CLASS_UNASSIGNED 0xFF

/** @brief Capability ID für MSI (Message Signaled Interrupts) */
#define PCI_CAP_ID_MSI 0x05
/** @brief Capability ID für MSI-X */
#define PCI_CAP_ID_MSIX 0x11

/**
 * @brief Repräsentiert ein erkanntes PCI/PCIe-Gerät im System.
 */
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

/**
 * @brief Funktionszeiger für die Probe-Funktion eines Treibers.
 * @param dev Zeiger auf das zu prüfende PCI-Gerät.
 * @return true, wenn der Treiber das Gerät erfolgreich übernommen hat.
 */
typedef bool (*pci_driver_probe_t)(pci_device_t *dev);

/**
 * @brief Repräsentiert einen PCI-Treiber und seine Abgleichkriterien.
 */
typedef struct pci_driver {
    const char *name;         /**< Name des Treibers */
    uint16_t vendor_id;       /**< Ziel-Hersteller-ID (0xFFFF für Beliebig/Wildcard) */
    uint16_t device_id;       /**< Ziel-Geräte-ID (0xFFFF für Beliebig/Wildcard) */
    uint8_t class_code;       /**< Ziel-Geräteklasse (0xFF für Beliebig/Wildcard) */
    uint8_t subclass;         /**< Ziel-Unterklasse (0xFF für Beliebig/Wildcard) */
    pci_driver_probe_t probe; /**< Callback-Funktion zur Initialisierung des Geräts */
    struct pci_driver *next;  /**< Zeiger auf den nächsten Treiber in der Liste */
} pci_driver_t;

/**
 * @brief Initialisiert das PCI-Subsystem und scannt den Bus mittels MCFG/ECAM.
 */
void pci_init();

/**
 * @brief Liefert den Kopf der Liste aller erkannten PCI-Geräte zurück.
 * @return Zeiger auf das erste `pci_device_t` der Liste.
 */
pci_device_t *pci_get_devices();

/**
 * @brief Registriert einen neuen Treiber im System und gleicht ihn mit vorhandenen Geräten ab.
 * @param driver Zeiger auf die zu registrierende Treiber-Struktur.
 */
void pci_register_driver(pci_driver_t *driver);

/**
 * @brief Liest einen 32-Bit-Wert aus dem Konfigurationsraum eines PCI-Geräts.
 * @param dev Zeiger auf das PCI-Gerät.
 * @param offset Byte-Offset im Konfigurationsraum.
 * @return Der gelesene 32-Bit Wert.
 */
uint32_t pci_read32(pci_device_t *dev, uint16_t offset);

/**
 * @brief Liest einen 16-Bit-Wert aus dem Konfigurationsraum eines PCI-Geräts.
 * @param dev Zeiger auf das PCI-Gerät.
 * @param offset Byte-Offset im Konfigurationsraum.
 * @return Der gelesene 16-Bit Wert.
 */
uint16_t pci_read16(pci_device_t *dev, uint16_t offset);

/**
 * @brief Liest einen 8-Bit-Wert aus dem Konfigurationsraum eines PCI-Geräts.
 * @param dev Zeiger auf das PCI-Gerät.
 * @param offset Byte-Offset im Konfigurationsraum.
 * @return Der gelesene 8-Bit Wert.
 */
uint8_t pci_read8(pci_device_t *dev, uint16_t offset);

/**
 * @brief Schreibt einen 32-Bit-Wert in den Konfigurationsraum eines PCI-Geräts.
 * @param dev Zeiger auf das PCI-Gerät.
 * @param offset Byte-Offset im Konfigurationsraum.
 * @param val Der zu schreibende 32-Bit Wert.
 */
void pci_write32(pci_device_t *dev, uint16_t offset, uint32_t val);

/**
 * @brief Schreibt einen 16-Bit-Wert in den Konfigurationsraum eines PCI-Geräts.
 * @param dev Zeiger auf das PCI-Gerät.
 * @param offset Byte-Offset im Konfigurationsraum.
 * @param val Der zu schreibende 16-Bit Wert.
 */
void pci_write16(pci_device_t *dev, uint16_t offset, uint16_t val);

/**
 * @brief Schreibt einen 8-Bit-Wert in den Konfigurationsraum eines PCI-Geräts.
 * @param dev Zeiger auf das PCI-Gerät.
 * @param offset Byte-Offset im Konfigurationsraum.
 * @param val Der zu schreibende 8-Bit Wert.
 */
void pci_write8(pci_device_t *dev, uint16_t offset, uint8_t val);

/**
 * @brief Aktiviert Message Signaled Interrupts (MSI) für das angegebene PCI-Gerät.
 * @param dev Zeiger auf das PCI-Gerät.
 * @param vector Der zuzuordnende Interrupt-Vektor IDT.
 * @param lapic_id Ziel-Local APIC ID für das Delivery Target.
 * @return true bei erfolgreicher Konfiguration, sonst false.
 */
bool pci_enable_msi(pci_device_t *dev, uint8_t vector, uint8_t lapic_id);