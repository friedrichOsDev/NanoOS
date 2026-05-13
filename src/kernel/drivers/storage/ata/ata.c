/**
 * @file ata.c
 * @author friedrichOsDev
 */

#include <ata.h>
#include <io.h>
#include <memory.h>
#include <string.h>
#include <serial.h>

static void ata_delay() {
    for(int i = 0; i < 4; i++) inb(ATA_STATUS);
}

static uint8_t ata_wait_ready() {
    while (inb(ATA_STATUS) & ATA_SR_BSY);
    uint8_t status = inb(ATA_STATUS);
    if (status & ATA_SR_ERR) return 0;
    return (status & ATA_SR_DRQ);
}

void ata_soft_reset() {
    outb(0x3F6, 0x04); // Software Reset Bit setzen
    ata_delay();
    outb(0x3F6, 0x00); // Zurück auf Normalbetrieb
    ata_delay();

    uint32_t timeout = 1000000;
    while (timeout > 0) {
        uint8_t status = inb(ATA_STATUS);
        
        if (!(status & ATA_SR_BSY)) {
            break; 
        }
        timeout--;
    }

    if (timeout == 0) {
        serial_printf("ATA: Soft reset timeout (BSY stuck)!\n");
    } else {
        serial_printf("ATA: Reset successful, status: %x\n", inb(ATA_STATUS));
    }
}

void ata_init() {
    serial_printf("ATA: Checking for controller...\n");
    
    uint8_t status = inb(ATA_STATUS);
    if (status == 0xFF) {
        serial_printf("ATA: No controller found (Floating Bus). Skipping.\n");
        return;
    }

    serial_printf("ATA: Controller found, resetting...\n");
    ata_soft_reset();
    serial_printf("ATA: Soft reset complete\n");
    ata_identify(0xA0); // Master
    serial_printf("ATA: Master identified\n");
    ata_identify(0xB0); // Slave
    serial_printf("ATA: Slave identified\n");
}

void ata_identify(uint8_t drive) {
    outb(ATA_DRIVE_SELECT, drive); // 0xA0 = Master
    ata_delay();

    if (inb(ATA_STATUS) == 0) {
        serial_printf("ATA: No drive on port %x\n", drive);
        return;
    }

    outb(ATA_SECTOR_COUNT, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);
    
    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);
    ata_delay();

    uint8_t status = inb(ATA_STATUS);
    if (status == 0) return;

    if (!ata_wait_ready()) {
        serial_printf("ATA: Drive %x did not respond to IDENTIFY\n", drive);
        return;
    }

    uint16_t data[256];
    insw(ATA_DATA, data, 256);

    uint32_t sectors = *((uint32_t*)&data[60]);

    ata_disk_t* ata_disk = (ata_disk_t*)kmalloc(sizeof(ata_disk_t));
    disk_t* new_disk = &ata_disk->base;
    ata_disk->drive_id = drive;
    strncpy(new_disk->name, (drive == 0xA0) ? "hda" : "hdb", 4);
    new_disk->total_sectors = sectors;
    new_disk->sector_size = 512;
    new_disk->type = TYPE_ATA;
    new_disk->read = ata_read_sectors;
    new_disk->write = ata_write_sectors; 

    storage_register_disk(new_disk);
    serial_printf("ATA: Registered disk %s with %u sectors\n", new_disk->name, new_disk->total_sectors);
}

uint8_t ata_read_sectors(disk_t* self, uint64_t lba, uint32_t count, void* buffer) {
    uint16_t* ptr = (uint16_t*)buffer;
    ata_disk_t* ata_disk = (ata_disk_t*)self;
    uint8_t drive_head = (ata_disk->drive_id == 0xA0) ? 0xE0 : 0xF0;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t current_lba = (uint32_t)lba + i;

        outb(ATA_DRIVE_SELECT, drive_head | ((current_lba >> 24) & 0x0F));
        
        outb(ATA_SECTOR_COUNT, 1); 
        outb(ATA_LBA_LOW,  (uint8_t)(current_lba));
        outb(ATA_LBA_MID,  (uint8_t)(current_lba >> 8));
        outb(ATA_LBA_HIGH, (uint8_t)(current_lba >> 16));

        outb(ATA_COMMAND, ATA_CMD_READ_PIO);

        ata_delay();
        if (!ata_wait_ready()) {
            serial_printf("ATA: Error reading sector %u from disk %s\n", (uint32_t)lba + i, self->name);
            return 1;
        }

        insw(ATA_DATA, ptr + (i * 256), 256);
    }

    return 0;
}

uint8_t ata_write_sectors(disk_t* self, uint64_t lba, uint32_t count, const void* buffer) {
    uint16_t* ptr = (uint16_t*)buffer;
    ata_disk_t* ata_disk = (ata_disk_t*)self;
    uint8_t drive_head = (ata_disk->drive_id == 0xA0) ? 0xE0 : 0xF0;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t current_lba = (uint32_t)lba + i;

        outb(ATA_DRIVE_SELECT, drive_head | ((current_lba >> 24) & 0x0F));
        outb(ATA_SECTOR_COUNT, 1);
        outb(ATA_LBA_LOW,  (uint8_t)(current_lba));
        outb(ATA_LBA_MID,  (uint8_t)(current_lba >> 8));
        outb(ATA_LBA_HIGH, (uint8_t)(current_lba >> 16));

        outb(ATA_COMMAND, ATA_CMD_WRITE_PIO);

        ata_delay();
        if (!ata_wait_ready()) {
            serial_printf("ATA: Error writing sector %u to disk %s\n", (uint32_t)lba + i, self->name);
            return 1; 
        }

        outsw(ATA_DATA, ptr + (i * 256), 256);

    }
    
    outb(ATA_COMMAND, ATA_CMD_CACHE_FLUSH);
    ata_delay();
    ata_wait_ready();

    return 0;
}