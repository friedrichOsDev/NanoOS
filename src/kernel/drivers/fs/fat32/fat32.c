/**
 * @file fat32.c
 * @author friedrichOsDev
 */

#include <fat32.h>
#include <heap.h>
#include <serial.h>
#include <string.h>

static uint32_t cluster_to_lba(fat32_fs_t* fs, uint32_t cluster) {
    return fs->data_lba + (cluster - 2) * fs->sectors_per_cluster;
}

static uint32_t get_fat_entry(fat32_fs_t* fs, uint32_t cluster) {
    uint32_t fat_offset = cluster * 4; // FAT32 uses 4 bytes per entry
    uint32_t fat_sector = fs->fat_lba + (fat_offset / fs->disk->sector_size);
    uint32_t entry_offset = fat_offset % fs->disk->sector_size;

    uint8_t sector_buffer[fs->disk->sector_size];
    storage_read(fs->disk, fat_sector, 1, sector_buffer);

    return *(uint32_t*)(sector_buffer + entry_offset) & 0x0FFFFFFF;
}

static void read_cluster(fat32_fs_t* fs, uint32_t cluster, void* buffer) {
    uint32_t lba = cluster_to_lba(fs, cluster);
    storage_read(fs->disk, lba, fs->sectors_per_cluster, buffer);
}

static void name_83_to_ascii(const char* name83, char* out_name) {
    int j = 0;
    for (int i = 0; i < 8 && name83[i] != ' '; i++) {
        out_name[j++] = name83[i];
    }
    if (name83[8] != ' ') {
        out_name[j++] = '.';
        for (int i = 8; i < 11 && name83[i] != ' '; i++) {
            out_name[j++] = name83[i];
        }
    }
    out_name[j] = '\0';
}

fat32_fs_t* fat32_mount(disk_t* disk, uint32_t partition_start_lba) {
    uint8_t sector_buffer[disk->sector_size];
    uint8_t res = storage_read(disk, partition_start_lba, 1, sector_buffer);
    
    if (res != 0) {
        serial_printf("FAT32: Failed to read boot sector\n");
        return NULL;
    }

    serial_printf("FAT32: Successfully read boot sector from LBA %u\n", partition_start_lba);
    
    fat32_bpb_t* bpb = (fat32_bpb_t*)sector_buffer;
    if (bpb->signature != 0xAA55) {
        serial_printf("FAT32: Invalid boot sector signature %x\n", bpb->signature);
        return NULL;
    }

    fat32_fs_t* fs = (fat32_fs_t*)kmalloc(sizeof(fat32_fs_t));
    if (!fs) {
        serial_printf("FAT32: Failed to allocate memory for filesystem structure\n");
        return NULL;
    }

    fs->disk = disk;
    fs->bpb = *bpb;
    fs->partition_lba_start = partition_start_lba;
    fs->fat_lba = partition_start_lba + bpb->reserved_sector_count;
    fs->data_lba = fs->fat_lba + (bpb->num_fats * bpb->fat_size_32);
    fs->sectors_per_cluster = bpb->sectors_per_cluster;
    fs->bytes_per_cluster = bpb->sectors_per_cluster * disk->sector_size;
    fs->root_dir_cluster = bpb->root_cluster;
    fs->total_clusters = (bpb->total_sectors_32 - (bpb->reserved_sector_count + (bpb->num_fats * bpb->fat_size_32))) / bpb->sectors_per_cluster;

    serial_printf("FAT32: Mounted filesystem with %u total clusters\n", fs->total_clusters);
    
    fat32_list_dir_t* dir_list = fat32_list_dir(fs, fs->root_dir_cluster);
    if (dir_list) {
        serial_printf("FAT32: Root directory contents:\n");
        for (uint32_t i = 0; i < dir_list->folder_count; i++) {
            serial_printf("  [DIR]  %s (Cluster: %u)\n", dir_list->folders[i].name, dir_list->folders[i].cluster);
            
            // List contents of each subfolder in the root directory
            fat32_list_dir_t* sub_dir = fat32_list_dir(fs, dir_list->folders[i].cluster);
            if (sub_dir) {
                for (uint32_t j = 0; j < sub_dir->folder_count; j++) {
                    serial_printf("    [DIR]  %s/%s\n", dir_list->folders[i].name, sub_dir->folders[j].name);
                }
                for (uint32_t j = 0; j < sub_dir->file_count; j++) {
                    // Read and print file content for debugging
                    serial_printf("    [FILE] %s/%s (Size: %u bytes, Cluster: %u)\n", dir_list->folders[i].name, sub_dir->files[j].name, sub_dir->files[j].size, sub_dir->files[j].cluster);
                    uint8_t* file_buf = (uint8_t*)kmalloc(sub_dir->files[j].size + 1);
                    if (file_buf) {
                        fat32_entry_t entry = {0};
                        entry.first_cluster_low = sub_dir->files[j].cluster & 0xFFFF;
                        entry.first_cluster_high = (sub_dir->files[j].cluster >> 16) & 0xFFFF;
                        entry.file_size = sub_dir->files[j].size;
                        fat32_read_file(fs, &entry, file_buf);
                        file_buf[sub_dir->files[j].size] = '\0';
                        serial_printf("--------- CONTENT START ---------\n%s\n--------- CONTENT END -----------\n", (char*)file_buf);
                        kfree((virt_addr_t)file_buf);
                    }
                }
                fat32_free_list_dir(sub_dir);
            }
        }
        for (uint32_t i = 0; i < dir_list->file_count; i++) {
            // Read and print file content for debugging
            serial_printf("  [FILE] %s (Size: %u bytes, Cluster: %u)\n", dir_list->files[i].name, dir_list->files[i].size, dir_list->files[i].cluster);
            uint8_t* file_buf = (uint8_t*)kmalloc(dir_list->files[i].size + 1);
            if (file_buf) {
                fat32_entry_t entry = {0};
                entry.first_cluster_low = dir_list->files[i].cluster & 0xFFFF;
                entry.first_cluster_high = (dir_list->files[i].cluster >> 16) & 0xFFFF;
                entry.file_size = dir_list->files[i].size;
                fat32_read_file(fs, &entry, file_buf);
                file_buf[dir_list->files[i].size] = '\0';
                serial_printf("--------- CONTENT START ---------\n%s\n--------- CONTENT END -----------\n", (char*)file_buf);
                kfree((virt_addr_t)file_buf);
            }
        }
        fat32_free_list_dir(dir_list);
    }

    return fs;
}

void fat32_umount(fat32_fs_t* fs) {
    kfree((virt_addr_t)fs);
    serial_printf("FAT32: Unmounted filesystem\n");
}

fat32_list_dir_t* fat32_list_dir(fat32_fs_t* fs, uint32_t cluster) {

    // Setup structures

    uint8_t* cluster_buffer = (uint8_t*)kmalloc(fs->bytes_per_cluster);
    if (!cluster_buffer) {
        serial_printf("FAT32: Failed to allocate memory for directory listing\n");
        return NULL;
    }

    fat32_list_dir_t* result = (fat32_list_dir_t*)kmalloc(sizeof(fat32_list_dir_t));
    if (!result) {
        serial_printf("FAT32: Failed to allocate memory for directory listing result\n");
        kfree((virt_addr_t)cluster_buffer);
        return NULL;
    }

    result->entry_count = 0;
    result->folder_count = 0;
    result->file_count = 0;
    result->folders = NULL;
    result->files = NULL;

    uint32_t current_cluster = cluster;

    // Count entries for kmalloc

    while (current_cluster < 0x0FFFFFF8) {
        read_cluster(fs, current_cluster, cluster_buffer);
        uint32_t entries_per_cluster = fs->bytes_per_cluster / sizeof(fat32_entry_t);
        fat32_entry_t* entries = (fat32_entry_t*)cluster_buffer;

        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            if (entries[i].name[0] == 0x00) break; // No more entries
            if ((uint8_t)entries[i].name[0] == 0xE5) continue; // Skip deleted entries
            if (entries[i].attr == 0x0F) continue; // Skip long filename entries

            if (entries[i].attr & 0x10) result->folder_count++;
            else result->file_count++;
        }
        current_cluster = get_fat_entry(fs, current_cluster);
    }

    result->entry_count = result->folder_count + result->file_count;
    if (result->entry_count == 0) {
        kfree((virt_addr_t)cluster_buffer);
        return result;
    }

    if (result->folder_count > 0) {
        result->folders = (fat32_folder_t*)kmalloc(result->folder_count * sizeof(fat32_folder_t));
    }
    if (result->file_count > 0) {
        result->files = (fat32_file_t*)kmalloc(result->file_count * sizeof(fat32_file_t));
    }

    // Fill Arrays with data

    current_cluster = cluster;
    uint32_t current_folder_index = 0;
    uint32_t current_file_index = 0;

    while (current_cluster < 0x0FFFFFF8) {
        read_cluster(fs, current_cluster, cluster_buffer);
        uint32_t entries_per_cluster = fs->bytes_per_cluster / sizeof(fat32_entry_t);
        fat32_entry_t* entries = (fat32_entry_t*)cluster_buffer;

        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            if (entries[i].name[0] == 0x00) break; // No more entries
            if ((uint8_t)entries[i].name[0] == 0xE5) continue; // Skip deleted entries
            if (entries[i].attr == 0x0F) continue; // Skip long filename entries

            char namebuf[256];
            name_83_to_ascii(entries[i].name, namebuf);

            uint32_t file_cluster = ((uint32_t)entries[i].first_cluster_high << 16) | entries[i].first_cluster_low;
            
            if (entries[i].attr & 0x10) {
                // Folder
                strcpy(result->folders[current_folder_index].name, namebuf);
                result->folders[current_folder_index].cluster = file_cluster;
                current_folder_index++;
            } else {
                // File
                strcpy(result->files[current_file_index].name, namebuf);
                result->files[current_file_index].cluster = file_cluster;
                result->files[current_file_index].size = entries[i].file_size;
                current_file_index++;
            }
        }
        current_cluster = get_fat_entry(fs, current_cluster);
    }

    kfree((virt_addr_t)cluster_buffer);
    return result;
}

void fat32_free_list_dir(fat32_list_dir_t* list) {
    if (!list) return;
    if (list->folders) kfree((virt_addr_t)list->folders);
    if (list->files) kfree((virt_addr_t)list->files);
    kfree((virt_addr_t)list);
}

void fat32_read_file(fat32_fs_t* fs, fat32_entry_t* entry, uint8_t* buffer) {
    if (!fs || !entry || !buffer) {
        serial_printf("FAT32: read_file error: invalid parameters\n");
        return;
    }

    uint32_t current_cluster = ((uint32_t)entry->first_cluster_high << 16) | entry->first_cluster_low;
    uint32_t bytes_left = entry->file_size;
    uint8_t* write_ptr = buffer;

    serial_printf("FAT32: Reading file: size %u, start cluster %u\n", bytes_left, current_cluster);

    // Temporärer Puffer, falls wir am Ende weniger als einen vollen Cluster lesen müssen
    uint8_t* cluster_buffer = (uint8_t*)kmalloc(fs->bytes_per_cluster);
    if (!cluster_buffer) return;

    while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8 && bytes_left > 0) {
        // Gesamten Cluster einlesen
        read_cluster(fs, current_cluster, cluster_buffer);

        // Berechnen, wie viel aus diesem Cluster kopiert werden muss
        uint32_t to_copy = (bytes_left > fs->bytes_per_cluster) ? fs->bytes_per_cluster : bytes_left;
        
        // In den Zielbuffer schieben
        memcpy(write_ptr, cluster_buffer, to_copy);
        
        write_ptr += to_copy;
        bytes_left -= to_copy;

        // Nächsten Cluster aus der FAT erfragen
        current_cluster = get_fat_entry(fs, current_cluster);
    }

    if (bytes_left > 0 && current_cluster >= 0x0FFFFFF8) {
        serial_printf("FAT32: Warning: reached end of cluster chain but %u bytes remain\n", bytes_left);
    }

    kfree((virt_addr_t)cluster_buffer);
}