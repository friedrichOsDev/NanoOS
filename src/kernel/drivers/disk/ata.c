/*
 * @file ata.c
 * @brief ATA (Advanced Technology Attachment) disk driver
 * @author friedrichOsDev
 */

#include <ata.h>
#include <io.h>
#include <serial.h>

/*
 * TODO: use acpi to get the hardware topology
 */

/*
 * Global ATA bus information
 */
ata_bus_info_t buses[2] = {
    {ATA_PRIMARY_IO, ATA_PRIMARY_CTRL, 14},
    {ATA_SECONDARY_IO, ATA_SECONDARY_CTRL, 15}
};

/*
 * Wait ca. 400ns for ATA operations
 * @param bus The ATA bus (primary or secondary)
 */
static void ata_io_wait(ATA_BUS bus) {
    uint16_t ctrl = buses[bus].ctrl_base;
    for (int i = 0; i < 4; i++) {
        inb(ctrl);
    }
}

/*
 * Wait until the BSY flag is cleared
 * @param bus The ATA bus (primary or secondary)
 * @return 0 on success, 1 on timeout
 */
static int ata_wait_bsy(ATA_BUS bus) {
    uint16_t io = buses[bus].io_base;
    for (int i = 0; i < 100000; i++) {
        if (!(inb(io + 7) & 0x80)) return 0;
    }
    return 1;
}

/*
 * Wait until the DRQ flag is set
 * @param bus The ATA bus (primary or secondary)
 * @return 0 on success, 1 on timeout
 */
static int ata_wait_drq(ATA_BUS bus) {
    uint16_t io = buses[bus].io_base;
    for (int i = 0; i < 100000; i++) {
        uint8_t status = inb(io + 7);
        if (status & 0x01) return 1; 
        if (status & 0x08) return 0;
    }
    return 1;
}

/*
 * Identify an ATA drive
 * @param bus The ATA bus (primary or secondary)
 * @param drive The ATA drive type (master or slave)
 * @return 0 on success, 1 on failure
 */
static uint8_t ata_identify(ATA_BUS bus, ATA_DRIVE_TYPE drive) {
    uint16_t io = buses[bus].io_base;

    outb(io + 6, (drive == ATA_DRIVE_MASTER) ? 0xA0 : 0xB0); // select drive
    ata_io_wait(bus);

    // clear all registers
    outb(io + 2, 0);
    outb(io + 3, 0);
    outb(io + 4, 0);
    outb(io + 5, 0);

    outb(io + 7, 0xEC); // command: identify drive
    ata_io_wait(bus);

    uint8_t status = inb(io + 7);
    if (status == 0) return 1; // no device

    if (ata_wait_bsy(bus)) return 1;

    if (inb(io + 4) != 0 || inb(io + 5) != 0) return 1; // not ATA device, could be ATAPI (CD-ROM)

    if (ata_wait_drq(bus)) return 1;

    uint16_t info[256];
    insw(io, info, 256);

    uint32_t sectors = *((uint32_t*)(info + 60));
    serial_puts("ATA ");
    serial_puts((drive == ATA_DRIVE_MASTER) ? "Master" : "Slave");
    serial_puts(" Drive on ");
    serial_puts((bus == ATA_BUS_PRIMARY) ? "Primary" : "Secondary");
    serial_puts(" Bus: \nSectors: ");
    serial_put_int(sectors);
    serial_puts("\n");

    /*
     * TODO: register the disk in some disk management system
     */

    return 0;
}

/*
 * A function to initialize the ATA driver
 * @param void
 */
void ata_init(void) {
    for (int bus = 0; bus < 2; bus++) {
        if (inb(buses[bus].io_base + 7) == 0xFF) continue; // no device present

        for (int drive = 0; drive < 2; drive++) {
            if (ata_identify(bus, drive)) {
                serial_puts("No ATA ");
                serial_puts((drive == ATA_DRIVE_MASTER) ? "Master" : "Slave");
                serial_puts(" Drive on ");
                serial_puts((bus == ATA_BUS_PRIMARY) ? "Primary" : "Secondary");
                serial_puts(" Bus\n");
                continue;
            }
        }
    }
}

/*
 * Read a sector from an ATA drive
 * @param bus The ATA bus (primary or secondary)
 * @param drive The ATA drive type (master or slave)
 * @param lba The logical block address to read from
 * @param buffer The buffer to store the read data
 * @return 0 on success, non-zero on failure
 */
uint8_t ata_read_sector(ATA_BUS bus, ATA_DRIVE_TYPE drive, uint32_t lba, uint8_t* buffer) {
    uint16_t io = buses[bus].io_base;

    outb(io + 6, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F)); // select drive and LBA mode
    ata_io_wait(bus);

    outb(io + 1, 0x00); // 0 for standard read
    outb(io + 2, 1); // sector count
    outb(io + 3, (uint8_t)lba); // LBA low byte
    outb(io + 4, (uint8_t)(lba >> 8)); // LBA mid byte
    outb(io + 5, (uint8_t)(lba >> 16)); // LBA high byte
    outb(io + 7, 0x20); // command: read sectors

    if (ata_wait_bsy(bus)) return 1;
    if (ata_wait_drq(bus)) return 1;

    insw(io, buffer, 256);
    return 0;
}

/*
 * Write a sector to an ATA drive
 * @param bus The ATA bus (primary or secondary)
 * @param drive The ATA drive type (master or slave)
 * @param lba The logical block address to write to
 * @param buffer The buffer containing the data to write
 * @return 0 on success, non-zero on failure
 */
uint8_t ata_write_sector(ATA_BUS bus, ATA_DRIVE_TYPE drive, uint32_t lba, uint8_t* buffer) {
    uint16_t io = buses[bus].io_base;

    outb(io + 6, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F)); // select drive and LBA mode
    ata_io_wait(bus);
    
    outb(io + 2, 1); // sector count
    outb(io + 3, (uint8_t)lba); // LBA low byte
    outb(io + 4, (uint8_t)(lba >> 8)); // LBA mid byte
    outb(io + 5, (uint8_t)(lba >> 16)); // LBA high byte
    outb(io + 7, 0x30); // command: write sectors

    if (ata_wait_bsy(bus)) return 1;
    if (ata_wait_drq(bus)) return 1;

    outsw(io, (uint16_t*)buffer, 256);

    outb(io + 7, 0xE7); // command: flush cache
    while (inb(buses[bus].ctrl_base) & 0x80); // wait until flush completes

    return 0;
}
