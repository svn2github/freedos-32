/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "detect.c" - Detection and initialization of disk devices      *
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

#include <errno.h>
#include "biosdisk.h"

//#define __DEBUG__
#ifdef __DEBUG__
  #define LOG_PRINTF(x) fd32_log_printf x
#else
  #define LOG_PRINTF(x)
#endif


/* Drive informations returned by standard INT 13h/AH=08h */
/* 'get drive parameters'.                                */
typedef struct
{
    unsigned bios_number;
    unsigned drive_type;
    unsigned cylinders;
    unsigned sectors;
    unsigned heads;
    unsigned number_of_drives;
    WORD     param_table_seg;
    WORD     param_table_off;
}
StdDriveInfo;


/* Informations returned by extended INT 13h/AH=41h */
/* 'IBM/MS Extensions - installation check'.        */
typedef struct
{
    BYTE  version;
    BYTE  internal;
    WORD  api_support;
    BYTE  ext_version;
}
ExtInstCheck;


/* Extended drive informations returned by extended INT 13h/AH=48h */
/* 'IBM/MS Extensions - get drive parameters'.                     */
typedef struct
{
    /* Fields present in version 1.x+ */
    WORD   buf_size;     /* 001Ah for v1.x, 001Eh for v2.x, 42h for v3.0     */
    WORD   info_flags;
    DWORD  phys_c;
    DWORD  phys_h;
    DWORD  phys_s;
    QWORD  total_sectors;
    WORD   bytes_per_sector;
    /* Fields present in version 2.0+ */
    DWORD  edd_params;   /* FFFFh:FFFFh if not available                     */
    /* Fields present in version 3.0 */
    WORD   signature;    /* BEDDh to indicate presence of Device Path info   */
    BYTE   dev_path_len; /* including signature and this byte (24h for v3.0) */
    BYTE   reserved[3];  /* 0                                                */
    char   host_bus[4];  /* ASCIZ name of host bus ("ISA" or "PCI")          */
    char   interface[8]; /* ASCIZ name of interface type                     */
    BYTE   int_path[8];  /* Interface Path (see #00275)                      */
    BYTE   dev_path[8];  /* Device Path (see #00276)                         */
    BYTE   reserved1;    /* 0                                                */
    BYTE   checksum;
}
__attribute__ ((packed)) ExtDriveInfo;


/* Device Parameter Table Extension returned by newer versions of      */
/* extended INT 13h/AH=48h 'IBM/MS Extensions - get drive parameters'. */
typedef struct
{
    WORD   base_port;    /* physical I/O port base address                    */
    WORD   control_port; /* disk-drive control port address                   */
    BYTE   flags;
    BYTE   proprietary;
    BYTE   irq;          /* bits 3-0; bits 7-4 reserved and must be 0         */
    BYTE   multi_sector; /* sector count for multi-sector transfers           */
    BYTE   dma_control;  /* bits 7-4: DMA type (0-2), bits 3-0: DMA channel   */
    BYTE   pio_control;  /* bits 7-4: reserved (0), bits 3-0: PIO type (1-4)  */
    WORD   options;
    WORD   reserved;     /* 0                                                 */
    BYTE   ext_revision; /* extension revision level                          */
    BYTE   checksum;
}
__attribute__ ((packed)) EddParams;


/* Wrapper function for standard INT 13h 'get disk type' service. */
/* On success, returns one of the following value:                */
/*  0 - Drive not present                                         */
/*  1 - Floppy without change-line support                        */
/*  2 - Floppy with change-line support                           */
/*  3 - Hard disk                                                 */
/* On failure, returns a negative number.                         */
static int get_disk_type(unsigned drive)
{
    FD32_DECLARE_REGS(regs);

    AH(regs) = 0x15; /* Disk - Get disk type */
    DL(regs) = (BYTE) drive;
    fd32_int(0x13, regs);
    if (FLAGS(regs) & 0x1) return -1; /* FIX ME: Convert BIOS error codes! */
    LOG_PRINTF(("[BIOSDISK] get_disk_type returned AX=%04xh\n", AX(regs)));
    return AH(regs);
}


/* Gets drive parameters using the standard BIOS service INT 13h/AH=08h. */
/* On success, returns 0 and fills the Std structure.                    */
/* Returns nonzero on failure.                                           */
static int std_driveinfo(unsigned drive, StdDriveInfo *std)
{
    FD32_DECLARE_REGS(regs);

    AH(regs) = 0x08;  /* Disk - get drive parameters */
    DL(regs) = (BYTE) drive;
    ES(regs) = 0;     /* to guard against BIOS bugs  */
    DI(regs) = 0;     /* to guard against BIOS bugs  */
    fd32_int(0x13, regs);
    if (FLAGS(regs) & 0x1) return -1; //INT 13h error code in AH

    std->bios_number      = drive;
    std->drive_type       = BL(regs);
    std->cylinders        = CH(regs) + ((WORD) (CL(regs) & 0xC0) << 2) + 1;
    std->sectors          = CL(regs) & 0x3F;
    std->heads            = DH(regs) + 1;
    std->number_of_drives = DL(regs);
    std->param_table_seg  = ES(regs);
    std->param_table_off  = DI(regs);
    return 0;
}


/* Wrapper function for extended INT 13h 'extensions installation check'. */
/* Returns zero if extensions are installed, nonzero if not.              */
static int ext_instcheck(unsigned drive, ExtInstCheck *c)
{
    FD32_DECLARE_REGS(regs);

    AH(regs) = 0x41;
    BX(regs) = 0x55AA;
    DL(regs) = (BYTE) drive;
    fd32_int(0x13, regs);
    if ((FLAGS(regs) & 0x1) || (BX(regs) != 0xAA55)) return 1;
    c->version     = AH(regs);
    c->internal    = AL(regs);
    c->api_support = CX(regs);
    c->ext_version = DH(regs);
    return 0;
}


/* Gets drive parameters using the extended BIOS service INT 13h/AH=48h. */
/* On success, returns 0 and fills the P structure.                      */
/* Returns nonzero on failure.                                           */
static int ext_driveinfo(unsigned drive, Disk *d)
{
    FD32_DECLARE_REGS(regs);
    LOWMEM_ADDR   buf_selector;
    WORD          buf_segment, buf_offset;
    ExtInstCheck  chk;
    ExtDriveInfo  ext;
    EddParams     edd;

    memset(&ext, 0, sizeof(ExtDriveInfo));

    if (ext_instcheck(drive, &chk)) return 1;
    switch (chk.version)
    {
        case 0x01 : ext.buf_size = 0x1A; break;
        case 0x20 :
        case 0x21 : ext.buf_size = 0x1E; break;
        case 0x30 : ext.buf_size = 0x42; break;
        default   : LOG_PRINTF(("[BIOSDISK] Unknown extensions version %i\n", chk.version));
                    return -1;
    }

    buf_selector = fd32_dosmem_get(ext.buf_size, &buf_segment, &buf_offset);
    /* TODO: Check for ENOMEM */
    fd32_memcpy_to_lowmem(buf_selector, 0, &ext, ext.buf_size);
    /* IBM/MS INT 13 Extensions - GET DRIVE PARAMETERS */
    AH(regs) = 0x48;
    DL(regs) = drive;
    DS(regs) = buf_segment;
    SI(regs) = buf_offset;
    fd32_int(0x13, regs);
    if (FLAGS(regs) & 0x1)
    {
        LOG_PRINTF(("[BIOSDISK] Error while getting extended infos for drive %02xh\n", drive));
        fd32_dosmem_free(buf_selector, ext.buf_size);
        return -1;
    }
    fd32_memcpy_from_lowmem(&ext, buf_selector, 0, ext.buf_size);
    fd32_dosmem_free(buf_selector, ext.buf_size);
    d->priv_flags   = EXTAVAIL;
    d->phys_c       = ext.phys_c;
    d->phys_h       = ext.phys_h;
    d->phys_s       = ext.phys_s;
    d->block_size   = ext.bytes_per_sector;
    d->total_blocks = ext.total_sectors;

    /* Check if Device Parameter Table Extension is available (RBIL #00278) */
    if ((chk.version >= 0x20) && (chk.api_support & 0x04)
     && (ext.edd_params != 0xFFFFFFFF))
    {
        fd32_memcpy_from_lowmem(&edd, DOS_SELECTOR, ((ext.edd_params >> 16) << 4)
                                + (WORD) ext.edd_params, sizeof(EddParams));
        /* If Device Parameter Table Extension available detect disk channel */
        switch (edd.base_port & 0xFFF0)
        {
            case 0x1F0: d->priv_flags |= KNOWSPORTS;
                        if (edd.flags & 0x10) d->priv_flags |= SLAVE;
                        break;
            case 0x170: d->priv_flags |= KNOWSPORTS | SECONDARY;
                        if (edd.flags & 0x10) d->priv_flags |= SLAVE;
                        break;
            default:
                #ifdef BIOSDISK_SHOWMSG
                fd32_message("Unknown base port %04xh for drive %02x!\n", edd.base_port, drive);
                #endif
        }
    }
    return 0;
}


#ifdef BIOSDISK_SHOWMSG
static const char *floppy_types[] =
{
    "unknown", "360 KiB", "1200 KiB", "720 KiB", "1440 KiB", "unknown", "2880 KiB"
};
#endif

static int biosdisk_detect_fd(void)
{
    #ifdef BIOSDISK_FD32DEV
    char         *dev_name;
    #endif
    Disk         *d;
    StdDriveInfo  std;
    char          name[10];
    int           res;
    unsigned      disk = 0;

    do
    {
        if ((res = std_driveinfo(disk, &std)))
        {
            #ifdef BIOSDISK_SHOWMSG
            fd32_message("Error %i getting parameters for drive %02xh\n", res, disk);
            #endif
            return -1;
        }
        LOG_PRINTF(("[BIOSDISK] Drive info for fd%d reports %u drives\n", disk, std.number_of_drives));
        if (std.number_of_drives == 0) break;

        /* Allocate memory for the private data, ops and device structures */
        if ((d = (Disk *) fd32_kmem_get(sizeof(Disk))) == NULL)
        {
            LOG_PRINTF(("[BIOSDISK] Not enough memory for fd%d private data\n", disk));
            return -ENOMEM;
        }
        ksprintf(name, "fd%d", disk);

        /* Initialize the private data */
        d->open_count   = 0;
        d->first_sector = 0;
        d->bios_number  = std.bios_number;
        d->priv_flags   = 0;
        d->bios_c       = d->phys_c = std.cylinders;
        d->bios_h       = d->phys_h = std.heads;
        d->bios_s       = d->phys_s = std.sectors;
        d->block_size   = 512;
        d->total_blocks = std.cylinders * std.heads * std.sectors;
        d->type         = FD32_BIFLO;
        d->multiboot_id = 0x00FFFFFF | (disk << 24);

        /* Initialize and add removable block operations */
        res = get_disk_type(std.bios_number);
        switch (res)
        {
            case 1  : d->priv_flags = REMOVABLE; break;
            case 2  : d->priv_flags = REMOVABLE | CHANGELINE; break;
            default : LOG_PRINTF(("[BIOSDISK] Unknown floppy type %i for fd%d\n", res, disk));
                      d->priv_flags = REMOVABLE | CHANGELINE; break;
                      //fd32_kmem_free(D, sizeof(tDisk));
                      return -1;
        }
        #ifdef BIOSDISK_SHOWMSG
        fd32_message("BIOS drive %02xh is %s (%s)\n", std.bios_number, name,
                     (std.drive_type <= 6) ? floppy_types[std.drive_type] : "unknown");
        #endif

        #ifdef BIOSDISK_FD32DEV
        /* Register the new device to the FD32 kernel */
        dev_name = (char *) fd32_kmem_get(strlen(name) + 1);
        if (dev_name == NULL)
        {
            LOG_PRINTF(("[BIOSDISK] Not enough memory for fd%d device name\n", disk));
            return -ENOMEM;
        }
        strcpy(dev_name, name);
        res = fd32_dev_register(biosdisk_request, (void *) d, dev_name);
        if (res < 0)
        {
            LOG_PRINTF(("[BIOSDISK] Unable to register the fd%d device to the FD32 kernel\n", disk));
            return res;
        }
        #endif
    }
    while (++disk < std.number_of_drives);
    return 0;
}


static int biosdisk_detect_hd(void)
{
    #ifdef BIOSDISK_FD32DEV
    char         *dev_name;
    #endif
    Disk         *d;
    StdDriveInfo  std;
    char          name[4] = "hd ";
    int           res;
    unsigned      disk = 0x80;

    do
    {
        if ((res = std_driveinfo(disk, &std)))
        {
            #ifdef BIOSDISK_SHOWMSG
            fd32_message("Error %i getting parameters for drive %02xh\n", res, disk);
            #endif
            return -1;
        }
        LOG_PRINTF(("[BIOSDISK] Drive info for drive %02xh reports %u drives\n", disk, std.number_of_drives));
        if (std.number_of_drives == 0) break;

        if ((d = (Disk *) fd32_kmem_get(sizeof(Disk))) == NULL)
        {
            LOG_PRINTF(("[BIOSDISK] Not enough memory for drive %02xh private data\n", disk));
            return -ENOMEM;
        }

        /* Initialize the private data */
        d->open_count   = 0;
        d->first_sector = 0;
        d->bios_number  = std.bios_number;
        d->bios_c       = std.cylinders;
        d->bios_h       = std.heads;
        d->bios_s       = std.sectors;
        d->type         = FD32_BIGEN;
        d->multiboot_id = 0x00FFFFFF | (disk << 24);
        if (ext_driveinfo(disk, d))
        {
            /* If extensions are not available, use standard BIOS values */
            d->priv_flags   = 0;
            d->phys_c       = std.cylinders;
            d->phys_h       = std.heads;
            d->phys_s       = std.sectors;
            d->block_size   = 512;
            d->total_blocks = std.cylinders * std.heads * std.sectors;
        }

        /* Complete the device name */
        if (d->priv_flags & KNOWSPORTS)
        {
            name[2] = 'a';
            if (d->priv_flags & SECONDARY) name[2] = 'c';
            if (d->priv_flags & SLAVE)     name[2]++;
        }
        else name[2] = disk - 0x80 + 'a';

        #ifdef BIOSDISK_SHOWMSG
        if (d->priv_flags & EXTAVAIL)
        {
            if (d->priv_flags & KNOWSPORTS)
                fd32_message("BIOS drive %02xh is %s (%lu MiB) supports LBA and knows ports\n",
                             d->bios_number, name, (DWORD) ((QWORD) d->total_blocks * d->block_size / 1048576));
             else
                fd32_message("BIOS drive %02xh is %s (%lu MiB) supports LBA\n",
                             d->bios_number, name, (DWORD) ((QWORD) d->total_blocks * d->block_size / 1048576));
        }
        else fd32_message("BIOS drive %02xh is %s (%lu MiB)\n",
                          d->bios_number, name, (DWORD) ((QWORD) d->total_blocks * d->block_size / 1048576));
        LOG_PRINTF(("[BIOSDISK] Std CHS=%02lx,%02lx,%02lx Lrg CHS=%lu,%lu,%lu\n",
                    d->bios_c, d->bios_h, d->bios_s, d->phys_c, d->phys_h, d->phys_s));
        #endif

        #ifdef BIOSDISK_FD32DEV
        /* Register the new device to the FD32 kernel */
        dev_name = (char *) fd32_kmem_get(strlen(name) + 1);
        if (dev_name == NULL)
        {
            LOG_PRINTF(("[BIOSDISK] Not enough memory for %s device name\n", name));
            return -ENOMEM;
        }
        strcpy(dev_name, name);
        res = fd32_dev_register(biosdisk_request, (void *) d, dev_name);
        if (res < 0)
        {
            LOG_PRINTF(("[BIOSDISK] Unable to register the %s device to the FD32 kernel\n", name));
            return res;
        }
        #endif
        biosdisk_scanpart(d, dev_name);
    }
    while (++disk - 0x80 < std.number_of_drives);
    return 0;
}


int biosdisk_detect(int fd, int hd)
{
//    int res;
//    if ((res = detect_floppies()) < 0) return res;
//    if ((res = detect_harddisks()) < 0) return res;
    if (fd) {
message("Detecting FD\n");
        biosdisk_detect_fd();
    }
    if (hd) {
message("Detecting HD\n");
        biosdisk_detect_hd();
    }

    return 0;
}


#if 0
/* This is to test the driver when compiled as a stand-alone program */
int main()
{
    return biosdisk_detect();
}
#endif
