/*
 * @file disk.c
 * @brief Generic disk driver interface
 * @author friedrichOsDev
 */

#include <disk.h>
#include <serial.h>

/*
 * Global disk array
 */
static disk_t* disks[10];

/*
 * Global disk count
 */
static int disk_count = 0;

/*
 * Register a new disk
 * @param d Pointer to the disk to register
 */
void disk_register(disk_t* d) {
    if (disk_count < 10) {
        disks[disk_count++] = d;
    } else {
        serial_puts("Maximum disk limit reached!\n");
    }
}

/*
 * Get a disk by its ID
 * @param disk_id The ID
 * @return Pointer to the disk structure or NULL if not found
 */
disk_t* get_disk(int disk_id) {
    if (disk_id >= disk_count) return NULL;
    return disks[disk_id];
}

/*
 * Get the total number of registered disks
 * @return The disk
 */
int get_disk_count(void) {
    return disk_count;
}

/*
 * Read a sector from a disk
 * @param disk_id The ID of the disk
 * @param lba The logical block address to read from
 * @param buffer The buffer to store the read data
 * @return 0 on success, -1 on failure
 */
int disk_read(int disk_id, uint32_t lba, uint8_t* buffer) {
    if (disk_id >= disk_count) return -1;
    return disks[disk_id]->ops->read_sector(disks[disk_id], lba, buffer);
}

/*
 * Write a sector to a disk
 * @param disk_id The ID of the disk
 * @param lba The logical block address to write to
 * @param buffer The buffer containing the data to write
 * @return 0 on success, -1 on failure
 */
int disk_write(int disk_id, uint32_t lba, uint8_t* buffer) {
    if (disk_id >= disk_count) return -1;
    return disks[disk_id]->ops->write_sector(disks[disk_id], lba, buffer);
}
