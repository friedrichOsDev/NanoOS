/**
 * @file fat32.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <storage.h>

/**
 * @brief FAT32 BIOS Parameter Block (BPB) structure.
 */
typedef struct {
    uint8_t  jmp_boot[3];           /**< Jump instruction to boot code */
    char     oem_name[8];           /**< OEM Name in ASCII */
    uint16_t bytes_per_sector;      /**< Bytes per logical sector */
    uint8_t  sectors_per_cluster;   /**< Logical sectors per cluster */
    uint16_t reserved_sector_count; /**< Reserved sectors in the reserved region */
    uint8_t  num_fats;              /**< Number of FATs (usually 2) */
    uint16_t root_entry_count;      /**< Unused in FAT32, should be 0 */
    uint16_t total_sectors_16;      /**< Unused in FAT32, should be 0 */
    uint8_t  media_type;            /**< Media descriptor */
    uint16_t fat_size_16;           /**< Unused in FAT32, should be 0 */
    uint16_t sectors_per_track;     /**< Sectors per track for interrupt 0x13 */
    uint16_t num_heads;             /**< Number of heads for interrupt 0x13 */
    uint32_t hidden_sectors;        /**< Count of hidden sectors */
    uint32_t total_sectors_32;      /**< Total count of sectors on the volume */
    uint32_t fat_size_32;           /**< Number of sectors per FAT */
    uint16_t fat_flags;             /**< FAT flags (mirroring, etc.) */
    uint16_t fs_version;            /**< FAT32 version number */
    uint32_t root_cluster;          /**< Cluster number of the root directory */
    uint16_t fs_info;               /**< Sector number of the FSInfo structure */
    uint16_t backup_boot_sector;    /**< Sector number of the backup boot sector */
    uint8_t  reserved[12];          /**< Reserved for future use */
    uint8_t  drive_number;          /**< BIOS drive number */
    uint8_t  reserved1;             /**< Reserved (used by Windows NT) */
    uint8_t  boot_signature;        /**< Extended boot signature (0x29) */
    uint32_t volume_id;             /**< Volume serial number */
    char     volume_label[11];      /**< Volume label string */
    char     fs_type[8];            /**< System identifier string (FAT32) */
    uint8_t  x86_boot_code[420];    /**< Boot code */
    uint16_t signature;             /**< Boot sector signature (0xAA55) */
} __attribute__((packed)) fat32_bpb_t;

/**
 * @brief FAT32 Directory Entry structure.
 */
typedef struct {
    char     name[11];           /**< 8.3 filename format (8 chars for name, 3 for extension) */
    uint8_t  attr;               /**< File attributes (e.g., read-only, hidden, system, volume label, directory, archive) */
    uint8_t  nt_reserved;        /**< Reserved for Windows NT, should be zero */
    uint8_t  creation_time_10ms; /**< Creation time in 10ms units (0-199) */
    uint16_t creation_time;      /**< Creation time (hours:5, minutes:6, seconds:5) */
    uint16_t creation_date;      /**< Creation date (year:7, month:4, day:5) */
    uint16_t last_access_date;   /**< Last access date (year:7, month:4, day:5) */
    uint16_t first_cluster_high; /**< High 16 bits of the first cluster number */
    uint16_t write_time;         /**< Last modification time (hours:5, minutes:6, seconds:5) */
    uint16_t write_date;         /**< Last modification date (year:7, month:4, day:5) */
    uint16_t first_cluster_low;  /**< Low 16 bits of the first cluster number */
    uint32_t file_size;          /**< File size in bytes */
} __attribute__((packed)) fat32_entry_t;

/**
 * @brief FAT32 Filesystem instance structure.
 */
typedef struct {
    disk_t* disk;               /**< Pointer to the underlying storage device */
    fat32_bpb_t bpb;            /**< BIOS Parameter Block containing volume metadata */
    uint32_t partition_lba_start; /**< The sector number where the data area begins */
    uint32_t fat_lba;           /**< The starting sector of the first File Allocation Table */
    uint32_t data_lba;          /**< Number of sectors occupied by one FAT */
    uint32_t sectors_per_cluster; /**< Number of sectors per cluster */
    uint32_t bytes_per_cluster; /**< Calculated size of a cluster in bytes */
    uint32_t root_dir_cluster;  /**< Cluster number where the root directory starts */
    uint32_t total_clusters;    /**< Total number of data clusters available on the volume */
} fat32_fs_t;

/**
 * @brief FAT32 Folder strukture
 */
typedef struct {
    char name[256];            /**< Name of the folder */
    uint32_t cluster;          /**< Starting cluster of the folder */
} fat32_folder_t;

/**
 * @brief FAT32 File structure
 */
typedef struct {
    char name[256];            /**< Name of the file */
    uint32_t cluster;          /**< Starting cluster of the file */
    uint32_t size;             /**< Size of the file in bytes */
} fat32_file_t;

/**
 * @brief FAT32 List DIR structure
 */
typedef struct {
    uint32_t entry_count;
    uint32_t folder_count;
    uint32_t file_count;
    fat32_folder_t* folders;
    fat32_file_t* files;
} fat32_list_dir_t;

fat32_fs_t* fat32_mount(disk_t* disk, uint32_t partition_start_lba);
void fat32_umount(fat32_fs_t* fs);
fat32_list_dir_t* fat32_list_dir(fat32_fs_t* fs, uint32_t cluster);
void fat32_free_list_dir(fat32_list_dir_t* list);
void fat32_read_file(fat32_fs_t* fs, fat32_entry_t* entry, uint8_t* buffer);