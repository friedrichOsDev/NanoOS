/**
 * @file storage.c
 * @brief The HAL for all storage drivers
 * @author friedrichOsDev
 */

#include <storage.h>
#include <serial.h>
#include <kernel.h>
#include <console.h>
#include <print.h>

static disk_t* disks[MAX_DISKS];
uint8_t disk_count = 0;

void storage_init() {
    for (uint8_t i = 0; i < MAX_DISKS; i++) {
        disks[i] = NULL;
    }
    init_state = INIT_STORAGE;
}

disk_t* storage_get_disk(uint8_t index) {
    if (index >= disk_count) {
        return NULL;
    }
    return disks[index];
}

void storage_register_disk(disk_t* disk) {
    if (disk_count < MAX_DISKS) {
        disks[disk_count++] = disk;
    }
}

uint8_t storage_get_disk_count() {
    return disk_count;
}

uint8_t storage_read(disk_t* disk, uint64_t lba, uint32_t count, void* buffer) {
    if (!disk || !disk->read) return 1;
    
    if (lba + count > disk->total_sectors) {
        serial_printf("Storage: Error: Out of bounds read at LBA %llu\n", lba);
        return 2; 
    }
    
    return disk->read(disk, lba, count, buffer);
}

uint8_t storage_write(disk_t* disk, uint64_t lba, uint32_t count, const void* buffer) {
    if (!disk || !disk->write) {
        serial_printf("Storage: Error: No disk or write function provided.\n");
        return 1;
    } 
    
    if (lba + count > disk->total_sectors) {
        serial_printf("Storage: Error: Out of bounds write at LBA %llu\n", lba);
        return 2; 
    }
    
    return disk->write(disk, lba, count, buffer);
}

void storage_dump_disk(disk_t* disk) {
    if (!disk) {
        console_puts(U"Storage: No disk provided.\n");
        return;
    }

    char buf[128];
    snprintf(buf, sizeof(buf), "  Name:           %s\n", disk->name);
    for (int i = 0; buf[i]; i++) console_putc((uint32_t)buf[i]);
    snprintf(buf, sizeof(buf), "  Type:           %s\n", disk->type == TYPE_ATA ? "ATA" : (disk->type == TYPE_AHCI ? "AHCI" : "Unknown"));
    for (int i = 0; buf[i]; i++) console_putc((uint32_t)buf[i]);
    snprintf(buf, sizeof(buf), "  Total Sectors:  %llu\n", disk->total_sectors);
    for (int i = 0; buf[i]; i++) console_putc((uint32_t)buf[i]);
    snprintf(buf, sizeof(buf), "  Sector Size:    %u bytes\n", disk->sector_size);
    for (int i = 0; buf[i]; i++) console_putc((uint32_t)buf[i]);
    snprintf(buf, sizeof(buf), "  Capacity:       %llu MB\n", (disk->total_sectors * disk->sector_size) / (1024 * 1024));
    for (int i = 0; buf[i]; i++) console_putc((uint32_t)buf[i]);
    snprintf(buf, sizeof(buf), "  Capabilities:   %s%s\n", disk->read ? "READ " : "", disk->write ? "WRITE" : "");
    for (int i = 0; buf[i]; i++) console_putc((uint32_t)buf[i]);
}

void storage_dump_info() {
    char buf[64];
    console_puts(U"Storage Devices:\n");
    snprintf(buf, sizeof(buf), "Total Disks: %u\n\n", disk_count);
    for (int i = 0; buf[i]; i++) console_putc((uint32_t)buf[i]);

    for (uint8_t i = 0; i < disk_count; i++) {
        snprintf(buf, sizeof(buf), "[Disk %u]\n", i);
        for (int j = 0; buf[j]; j++) console_putc((uint32_t)buf[j]);
        storage_dump_disk(disks[i]);
        console_putc(U'\n');
    }
}