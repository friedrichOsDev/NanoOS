/*
 * @file disk.h
 * @brief Header file for generic disk driver interface
 * @author friedrichOsDev
 */

#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stddef.h>

/*
 * disk types
 */
typedef enum {
    DISK_TYPE_ATA
} disk_type_t;

struct disk;

/*
 * disk operations structure
 */
typedef struct {
    int (*read_sector)(struct disk* disk, uint32_t lba, uint8_t* buffer);
    int (*write_sector)(struct disk* disk, uint32_t lba, uint8_t* buffer);
} disk_ops_t;

/*
 * disk structure
 */
typedef struct disk {
    char name[16];
    uint32_t sector_count;
    disk_type_t type;
    disk_ops_t* ops;
    void* device_specific;
} disk_t;

void disk_register(disk_t* d);
disk_t* get_disk(int disk_id);
int get_disk_count(void);
int disk_read(int disk_id, uint32_t lba, uint8_t* buffer);
int disk_write(int disk_id, uint32_t lba, uint8_t* buffer);

#endif // DISK_H