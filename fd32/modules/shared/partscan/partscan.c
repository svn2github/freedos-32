/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "partscan.c" - Hard disk partitions scanner                    *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 BIOSDisk Driver.                    *
 *                                                                        *
 * The FreeDOS32 BIOSDisk Driver is free software; you can redistribute   *
 * it and/or modify it under the terms of the GNU General Public License  *
 * as published by the Free Software Foundation; either version 2 of the  *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * The FreeDOS32 BIOSDisk Driver is distributed in the hope that it will  *
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty *
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the FreeDOS32 BIOSDisk Driver; see the file COPYING;        *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

/* Modifications (#define directives below) by Nils Labugt, Mach 2005 */
/* to make this file compile with the ATA driver  */


#define FD32DEV
#define SHOWMSG

#ifdef __BIOSDISK__

#include <errno.h>
#include "biosdisk.h"

#define PS_DEVICE Disk
#define PS_READ biosdisk_read
#define PS_HEADS bios_h
#define PS_SECTS bios_s
#define PS_BLOCK_SIZE block_size
#define PS_SCANPART biosdisk_scanpart
#define PS_REQUEST biosdisk_request
#define B_CONST const

#endif

#ifdef __ATA__

#include <dr-env.h>
#include <errno.h>
#include "ata.h"
#include "disk.h"
#include "partscan.h"
#include "ata-ops.h"

int ata_request(int function, ...); /* FIXME */
ssize_t ata_sp_read(void *handle, void *buffer, uint64_t start, size_t count, int flags);

#define PS_DEVICE struct ata_device
#define PS_READ ata_sp_read
#define PS_HEADS heads
#define PS_SECTS sectors
#define PS_BLOCK_SIZE bytes_per_sector
#define PS_SCANPART ata_scanpart
#define PS_REQUEST ata_request
#define B_CONST


#endif

#include "../../block/block.h" /* FIXME! */

typedef struct
{
    BYTE  active;
    BYTE  start_h;
    BYTE  start_s;
    BYTE  start_c;
    BYTE  type;
    BYTE  end_h;
    BYTE  end_s;
    BYTE  end_c;
    DWORD lba_start;
    DWORD lba_size;
}
__attribute__ ((packed)) PartTabl;


#define PART_EMPTY     0x00
#define PART_EXT_DOS   0x05
#define PART_EXT_WIN   0x0F
#define PART_EXT_LINUX 0x85

#ifdef SHOWMSG
static const struct
{
    BYTE id;
    char *name;
}
partition_types[] =
    {
        { 0x00, "Empty"             },
        { 0x01, "FAT12"             },
        { 0x04, "FAT16 up to 32 MB" },
        { 0x05, "DOS extended"      },
        { 0x06, "FAT16 over 32 MB"  },
        { 0x07, "NTFS or HPFS"      },
        { 0x0B, "FAT32"             },
        { 0x0C, "FAT32 with LBA"    },
        { 0x0E, "FAT16 with LBA"    },
        { 0x0F, "Win extended"      },
        { 0x82, "Linux Swap"        },
        { 0x83, "Linux native"      },
        { 0x85, "Linux extended"    },
        { 0xFF, "unknown"           },
        { 0, 0 }
    };


static char *partname(BYTE id)
{
    unsigned k;
    for (k = 0; partition_types[k].name; k++)
        if (partition_types[k].id == id)
            return partition_types[k].name;
    return "unknown";
}
#endif


static DWORD chs_to_lba(unsigned c, unsigned h, unsigned s, unsigned num_h, unsigned num_s)
{
    return (DWORD) ((c * num_h + h) * num_s + s - 1);
}


static void check_lba(PartTabl *p, const PS_DEVICE *d)
{
    DWORD lba_start, lba_size;

    if (d->total_blocks / (d->PS_HEADS * d->PS_SECTS) > 1024)
        return; /* ...since we trust them */
    lba_start = chs_to_lba(p->start_c + ((WORD) (p->start_s & 0xC0) << 2),
                           p->start_h, p->start_s & 0x3F, d->PS_HEADS, d->PS_SECTS);
    lba_size  = chs_to_lba(p->end_c + ((WORD) (p->end_s & 0xC0) << 2),
                           p->end_h, p->end_s & 0x3F, d->PS_HEADS, d->PS_SECTS)
                - lba_start + 1;
    if ((lba_start != p->lba_start) || (lba_size != p->lba_size))
        fd32_message("LBA values for the partition are not consistent with CHS values!\n");
    p->lba_start = lba_start;
    p->lba_size  = lba_size;
}


static int add_partition(const PS_DEVICE *d, const char *name, unsigned part, PartTabl *p)
{
#ifdef FD32DEV
    int    res;
    char  *dev_name;
#endif

    DWORD  lba_start = p->lba_start;
    DWORD  lba_end   = p->lba_start + p->lba_size - 1;
    DWORD  type;
    char   new_name[256];
    PS_DEVICE  *new_d;

    if ((p->type != PART_EMPTY)
            && (p->type != PART_EXT_DOS)
            && (p->type != PART_EXT_WIN)
            && (p->type != PART_EXT_LINUX))
    {
        ksprintf(new_name, "%s%u", name, part);
        type = BLOCK_DEVICE_INFO_TLOGICAL;
        if (part < 5)
        {
            if (p->active)
                type = BLOCK_DEVICE_INFO_TACTIVE;
            else
                type = BLOCK_DEVICE_INFO_TPRIMARY;
        }
        type |= (DWORD) p->type;
#ifdef SHOWMSG

        fd32_message("%s is %s (%02xh), Start: %lu, Size: %lu (%lu MiB)\n",
                     new_name, partname(p->type), (unsigned) p->type, lba_start,
                     lba_end - lba_start + 1,
                     (DWORD) (((long long) (lba_end - lba_start + 1) * d->PS_BLOCK_SIZE) >> 20));
#endif

        check_lba(p, d);

        /* Allocate and initialize the private data structure */
        if ((new_d = (PS_DEVICE *) fd32_kmem_get(sizeof(PS_DEVICE))) == NULL)
            return -ENOMEM;
        memcpy(new_d, d, sizeof(PS_DEVICE));
        new_d->open_count = 0;
        new_d->first_sector = lba_start;
        new_d->total_blocks = lba_end - lba_start + 1;
        new_d->type         = type;
        new_d->multiboot_id = (d->multiboot_id & 0xFF00FFFF) | ((part - 1) << 16);

#ifdef FD32DEV
        /* Register the new device to the FD32 kernel */
        dev_name = (char *) fd32_kmem_get(strlen(new_name) + 1);
        if (dev_name == NULL)
            return -ENOMEM;
        strcpy(dev_name, new_name);
        res = block_register(dev_name, PS_REQUEST, (void *) new_d);
        if (res < 0)
            return res;
#endif

    }
    return 0;
}


int PS_SCANPART(B_CONST PS_DEVICE *d, const char *dev_name)
{
    PartTabl  tbl[4];
    unsigned  k, part_num = 0;
    int       res;
    BYTE      buffer[d->PS_BLOCK_SIZE];
    DWORD     ext_start = 0, ext_start1 = 0;

    if (d->total_blocks / (d->PS_HEADS * d->PS_SECTS) > 1024)
    {
        fd32_message("Media is too big to be addressed using CHS (has more than 1024 cylinders).\n");
        fd32_message("LBA values in the partition tables will be trusted.\n");
    }
    else
    {
        fd32_message("Media can be addressed using CHS (has up to 1024 cylinders).\n");
        fd32_message("LBA values will be calculated from their respective CHS values.\n");
    }

    /* Read the MBR and copy the partition table in the tbl array */
    res = PS_READ((void *)d, buffer, 0, 1, 0/* flags */);
    if (res < 0)
        return res;
    memcpy(tbl, buffer + 446, 64);

    /* If there are no active partitions, mark the first primary as active */
    for (k = 0; k < 4; k++)
        if (tbl[k].active)
            break;
    if (k == 4)
        for (k = 0; k < 4; k++)
            if ((tbl[k].type != PART_EMPTY)
                    && (tbl[k].type != PART_EXT_DOS)
                    && (tbl[k].type != PART_EXT_WIN)
                    && (tbl[k].type != PART_EXT_LINUX))
            {
                tbl[k].active = 0x80;
                break;
            }
    /* Create partition devices for primary partitions */
    for (k = 0; k < 4; k++)
        add_partition(d, dev_name, ++part_num, &tbl[k]);
    /* Search for an extended partition in the primary partition table */
    for (k = 0; k < 4; k++)
    {
        if ((tbl[k].type == PART_EXT_DOS)
                || (tbl[k].type == PART_EXT_WIN)
                || (tbl[k].type == PART_EXT_LINUX))
        {
#ifdef SHOWMSG
            fd32_message("%s%u is a %s partition\n", dev_name, k + 1,
                         partname(tbl[k].type));
            check_lba(&tbl[k], d);
#endif

            ext_start = tbl[k].lba_start;
            res = PS_READ((void *)d, buffer, ext_start, 1, 0/* flags */);
            if (res < 0)
                return res;
            break;
        }
    }

    /* Read extended partitions */
    if (!ext_start)
        return 0;
    for (;;)
    {
        memcpy(tbl, buffer + 446, 64);
        for (k = 0; k < 4; k++)
        {
            if ((tbl[k].type != PART_EMPTY)
                    && (tbl[k].type != PART_EXT_DOS)
                    && (tbl[k].type != PART_EXT_WIN)
                    && (tbl[k].type != PART_EXT_LINUX))
            {
                part_num++;
                tbl[k].lba_start += ext_start + ext_start1;
            }
            add_partition(d, dev_name, part_num, &tbl[k]);
        }
        for (k = 0; k < 4; k++)
        {
            if ((tbl[k].type == PART_EXT_DOS)
                    || (tbl[k].type == PART_EXT_WIN)
                    || (tbl[k].type == PART_EXT_LINUX))
            {
                ext_start1 = tbl[k].lba_start;
                check_lba(&tbl[k], d);
                res = PS_READ((void *)d, buffer, ext_start + ext_start1, 1, 0/* flags */);
                if (res < 0)
                    return res;
                break;
            }
        }
        if (k == 4)
            break;
    }
    return 0;
}
