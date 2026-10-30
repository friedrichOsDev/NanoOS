#pragma once

#include <drivers/pci.h>
#include <pch.h>

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
 * @brief Liefert den Kopf der Liste aller erkannten PCI-Geräte zurück.
 * @return Zeiger auf das erste `pci_device_t` der Liste.
 */
pci_device_t *pci_get_devices();

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
