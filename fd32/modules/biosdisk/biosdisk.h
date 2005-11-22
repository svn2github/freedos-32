/* The FreeDOS-32 BIOSDisk Driver
 * a block device driver using the BIOS disk services.
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "biosdisk.h" is part of the FreeDOS-32 BIOSDisk Driver (the Program).
 *
 * The Program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Program; see the file GPL.txt; if not, write to
 * the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/** \file
 * Defines and declarations.
 */

#ifndef __FD32_BIOSDISK_H
#define __FD32_BIOSDISK_H

#include <dr-env.h>
#include <devices.h>

/* Use the following defines to add features to the BIOSDisk driver */
#define BIOSDISK_WRITE   /* Define this to enable write support           */
#define BIOSDISK_FD32DEV /* Define this to register FD32 devices          */
#define BIOSDISK_SHOWMSG /* Define this to show messages during detection */

/* Mnemonics for the private flags of the Disk structure */
enum
{
    EXTAVAIL   = 1 << 0, /* BIOS extensions are available  */
    KNOWSPORTS = 1 << 1, /* The BIOS knows drive ports     */
    SECONDARY  = 1 << 2, /* Device is on secondary channel */
    SLAVE      = 1 << 3, /* Device is slave                */
    REMOVABLE  = 1 << 4, /* Device has removable media     */
    CHANGELINE = 1 << 5  /* Change line is supported       */
};

/* BIOSDisk device structure */
typedef struct Disk Disk;
struct Disk
{
    unsigned open_count;
    DWORD    first_sector;
    unsigned bios_number;
    unsigned priv_flags;
    DWORD    bios_c, bios_h, bios_s;
    DWORD    phys_c, phys_h, phys_s;
    DWORD    block_size;   /* As defined in FD32_BLOCKINFO */
    DWORD    total_blocks; /* As defined in FD32_BLOCKINFO */
    DWORD    type;         /* As defined in FD32_BLOCKINFO */
    DWORD    multiboot_id; /* As defined in FD32_BLOCKINFO */
    /* Wrapper function for the BIOS transfer services */
    int (*xfer)(int operation, const Disk *d, DWORD lba, unsigned num_sectors);
    /* Sector-sized transfer buffer allocated in low memory */
    LOWMEM_ADDR tb_selector;
    WORD        tb_segment, tb_offset;
    /* 16-byte address packet allocated in low memory */
    LOWMEM_ADDR ap_selector;
    WORD        ap_segment, ap_offset;
};

/* common.c */
int biosdisk_std_xfer   (int operation, const Disk *d, DWORD lba, unsigned num_sectors);
int biosdisk_ext_xfer   (int operation, const Disk *d, DWORD lba, unsigned num_sectors);
ssize_t biosdisk_read(void *handle, void *buffer, uint64_t start, size_t count, int flags);
ssize_t biosdisk_write(void *handle, const void *buffer, uint64_t start, size_t count, int flags);
int biosdisk_mediachange(const Disk *d);
/* detect.c */
int biosdisk_detect     (int floppy, int hd);
/* partscan.c */
int  biosdisk_scanpart  (const Disk *d, const char *dev_name);
void biosdisk_timer     (void *p);
/* reflect.c */
void biosdisk_reflect   (unsigned intnum, BYTE dummy);
/* request.c */
int biosdisk_request(int function, ...);

#endif /* #ifndef __FD32_BIOSDISK_H */
