/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#ifndef __DISK_H__
#define __DISK_H__

struct ata_device
{
    DWORD bytes_per_sector;
    DWORD total_blocks;
    DWORD type;
    DWORD multiboot_id;
        
    DWORD first_sector;
    unsigned open_count;
    DWORD cyls;
    DWORD heads;
    DWORD sectors;
    
    char device_name[8];
    struct ide_interface *interface;
    
    int drv_number;
    WORD capabilities;
    char model[40+1];
    char serial[20+1];
    char revision[8+1];
    BYTE dev_bit;
    int polled_mode;
    int sectors_per_block;
    BYTE max_multiple_rw;
    BYTE status_reg;
    BYTE error_reg;
    int flags;
    WORD general_config;
    BYTE int_reason;
};

#endif
