/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "extdisk.c" - IBM/MS Extended BIOS disk functions              *
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

#include <dr-env.h>
#include "biosdisk.h"


/* Mnemonics for extended BIOS disk operations */
typedef enum
{
    EXT_READ   = 0x42,
    EXT_WRITE  = 0x43,
    EXT_VERIFY = 0x44
}
ExtOp;


/* Address packet to specify disk positions in extended INT13h services */
typedef struct
{
    BYTE   size;       /* Size of this structure: 10h or 18h                  */
    BYTE   reserved;   /* 0                                                   */
    WORD   num_blocks; /* Max 007Fh for Phoenix EDD                           */
    DWORD  buf_addr;   /* Seg:off pointer to transfer buffer                  */
    QWORD  start;      /* Starting absolute block number                      */
    QWORD  flat_addr;  /* (EDD-3.0, optional) 64-bit flat address of transfer */
                       /* buffer; used if DWORD at 04h is FFFFh:FFFFh         */
}
__attribute__ ((packed)) AddressPacket;


/* Wrapper function for extended INT13h 'read', 'write' and 'verify' */
/* services. Returns zero on success, nonzero on failure.            */
static inline int ext_disk_op(ExtOp operation, BYTE drive, DWORD lba,
                              BYTE num_sectors, WORD buf_seg, WORD buf_off)
{
    FD32_DECLARE_REGS(regs);
    AddressPacket ap;
    LOWMEM_ADDR   ap_selector;
    WORD          ap_segment, ap_offset;

    ap_selector = fd32_dosmem_get(sizeof(AddressPacket), &ap_segment, &ap_offset);
    ap.size       = 0x10;
    ap.reserved   = 0x00;
    ap.num_blocks = num_sectors;
    ap.buf_addr   = (buf_seg << 16) + buf_off;
    ap.start      = lba;
    ap.flat_addr  = 0;
    fd32_memcpy_to_lowmem(ap_selector, 0, &ap, sizeof(AddressPacket));

    AH(regs) = operation; /* Can be EXT_READ, EXT_WRITE or EXT_VERIFY */
    DL(regs) = drive;
    DS(regs) = ap_segment;
    SI(regs) = ap_offset;
    fd32_int(0x13, regs);

    fd32_dosmem_free(ap_selector, sizeof(AddressPacket));

    if (FLAGS(regs) & 0x1) return -1; //INT 13h error code in AH
    return 0;
}


/* Reads data from disk sectors into a user buffer, using the extended */
/* BIOS service INT 13h/AH=42h.                                        */
/* Returns zero on success, nonzero on failure.                        */
int biosdisk_extread(const Disk *d, DWORD start, DWORD count, void *buffer)
{
    int         res;
    LOWMEM_ADDR buf_selector;
    WORD        buf_segment, buf_offset;

    if (start >= d->total_blocks) return -1;
    buf_selector = fd32_dosmem_get(count * d->block_size, &buf_segment, &buf_offset);
    res = ext_disk_op(EXT_READ, d->bios_number, start + d->first_sector, count,
                      buf_segment, buf_offset);
    fd32_memcpy_from_lowmem(buffer, buf_selector, 0, count * d->block_size);
    fd32_dosmem_free(buf_selector, count * d->block_size);
    return res;
}


#ifdef BIOSDISK_WRITE
/* Writes data from a user buffer into disk sectors, using the extended */
/* BIOS service INT 13h/AH=43h.                                         */
/* Returns zero on success, nonzero on failure.                         */
int biosdisk_extwrite(const Disk *d, DWORD start, DWORD count, const void *buffer)
{
    int         res;
    LOWMEM_ADDR buf_selector;
    WORD        buf_segment, buf_offset;

    if (start >= d->total_blocks) return -1;
    buf_selector = fd32_dosmem_get(count * d->block_size, &buf_segment, &buf_offset);
    fd32_memcpy_to_lowmem(buf_selector, 0, (void *) buffer, count * d->block_size);
    res = ext_disk_op(EXT_WRITE, d->bios_number, start + d->first_sector, count,
                      buf_segment, buf_offset);
    fd32_dosmem_free(buf_selector, count * d->block_size);
    return res;
}
#endif
