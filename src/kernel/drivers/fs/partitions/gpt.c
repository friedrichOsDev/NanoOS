/**
 * @file gpt.c
 * @author friedrichOsDev
 */

#include <gpt.h>
#include <serial.h>
#include <storage.h>
#include <partman.h>

bool gpt_parse(uint8_t disk_index) {
    disk_t* disk = storage_get_disk(disk_index);
    if (!disk) {
        serial_printf("GPT: Error: No disk found at index %d\n", disk_index);
        return false;
    }

    gpt_header_t header;

    if (storage_read(disk, 1, 1, &header) != 0) {
        serial_printf("GPT: Error reading GPT header from disk %s\n", disk->name);
        return false;
    }

    if (header.signature != GPT_SIGNATURE) {
        serial_printf("GPT: Invalid GPT signature on disk %s: %X\n", disk->name, header.signature);
        return false;
    }

    serial_printf("GPT: Found GPT header on disk %s with signature %X\n", disk->name, header.signature);
    serial_printf("GPT: Disk GUID: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n", 
        header.disk_guid.data1, header.disk_guid.data2, header.disk_guid.data3,
        header.disk_guid.data4[0], header.disk_guid.data4[1], header.disk_guid.data4[2], header.disk_guid.data4[3],
        header.disk_guid.data4[4], header.disk_guid.data4[5], header.disk_guid.data4[6], header.disk_guid.data4[7]);
    serial_printf("GPT: First usable LBA: %llu, Last usable LBA: %llu\n",
        header.first_usable_lba,
        header.last_usable_lba);
    serial_printf("GPT: Entries start at LBA %llu, Total entries: %lu, Entry size: %lu\n", 
        header.partition_entry_lba, 
        header.num_partition_entries, 
        header.partition_entry_size);
    
    uint8_t sector_buffer[512];

    uint32_t entries_per_sector = 512 / header.partition_entry_size;
    if (entries_per_sector == 0) {
        serial_printf("GPT: Error: Invalid partition entry size %u\n", header.partition_entry_size);
        return false;
    }
    
    uint32_t current_lba = (uint32_t)header.partition_entry_lba;

    for (uint32_t i = 0; i < header.num_partition_entries; i++) {
        uint32_t sub_index = i % entries_per_sector;

        if (sub_index == 0) {
            if (storage_read(disk, current_lba, 1, sector_buffer) != 0) {
                serial_printf("GPT: Error reading partition entry sector %u\n", current_lba);
                return false;
            }
            current_lba++;
        }

        gpt_partition_entry_t* entry = (gpt_partition_entry_t*)(sector_buffer + (sub_index * header.partition_entry_size));

        if (entry->partition_type_guid.data1 == 0 && entry->partition_type_guid.data2 == 0) {
            continue;
        }

        uint64_t total_sectors = entry->ending_lba - entry->starting_lba + 1;

        char name_ascii[36];
        for (int j = 0; j < 35; j++) {
            name_ascii[j] = (char)entry->partition_name[j];
            if (name_ascii[j] == '\0') break;
        }
        name_ascii[35] = '\0';

        serial_printf("GPT: Found Partition %d: '%s' (Start LBA: %u, Size: %u sectors)\n", 
                      i, name_ascii, (uint32_t)entry->starting_lba, (uint32_t)total_sectors);

        partman_register_partition(
            disk_index, 
            (uint32_t)entry->starting_lba, 
            (uint32_t)total_sectors, 
            0x00,
            name_ascii
        );
    }

    return true;
}