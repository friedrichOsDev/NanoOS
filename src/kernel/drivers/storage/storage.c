/**
 * @file storage.c
 * @brief The HAL for all storage drivers
 * @author friedrichOsDev
 */

#include <storage.h>
#include <serial.h>
#include <kernel.h>

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
    if (!disk || !disk->write) return 1;
    
    if (lba + count > disk->total_sectors) {
        serial_printf("Storage: Error: Out of bounds write at LBA %llu\n", lba);
        return 2; 
    }
    
    return disk->write(disk, lba, count, buffer);
}

void storage_dump_info(disk_t* disk) {
    if (!disk) {
        serial_printf("Storage: No disk provided.\n");
        return;
    }

    serial_printf("Storage: Disk Name: %s\n", disk->name);
    serial_printf("Storage: Total Sectors: %llu\n", disk->total_sectors);
    serial_printf("Storage: Sector Size: %u bytes\n", disk->sector_size);
    serial_printf("Storage: Disk Type: %d\n", disk->type);
    serial_printf("Storage: Total Capacity: %llu MB\n", (disk->total_sectors * disk->sector_size) / (1024 * 1024));
    serial_printf("Storage: Read Support: %s\n", disk->read ? "Yes" : "No");
    serial_printf("Storage: Write Support: %s\n", disk->write ? "Yes" : "No");
}