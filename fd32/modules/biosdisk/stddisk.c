/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2005, Salvatore Isaja                               *
 *                                                                        *
 * This is "stddisk.c" - Standard BIOS disk functions                     *
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


/* Mnemonics for standard BIOS disk operations */
typedef enum
{
    STD_READ   = 0x02,
    STD_WRITE  = 0x03,
    STD_VERIFY = 0x04
}
StdOp;


/* A CHS triplet */
typedef struct { unsigned c, h, s; } Chs;


/* Convert LBA 32-bit absolute sector number in CHS coordinates. */
/* Return nonzero if LBA is too large for CHS rappresentation.   */
static inline int lba_to_chs(DWORD lba, Chs *chs, const Chs *total)
{
    DWORD    temp;
    unsigned bytes_per_cyl = total->h * total->s;
    if (lba >= total->c * bytes_per_cyl) return -1;
    chs->c   = lba / bytes_per_cyl;
    temp     = lba % bytes_per_cyl;
    chs->h   = temp / total->s;
    chs->s   = temp % total->s + 1;
    return 0;
}


/* Wrapper function for standard INT 13h 'read', 'write' and 'verify' */
/* services. Returns zero on success, nonzero on failure.             */
static inline int std_disk_op(StdOp operation, const Disk *d, DWORD lba,
                              BYTE num_sectors, WORD buf_seg, WORD buf_off)
{
    FD32_DECLARE_REGS(regs);
    Chs chs;
    Chs total = { d->bios_c, d->bios_h, d->bios_s };

    if (lba + num_sectors > d->total_blocks) return -1; /* Invalid LBA address */
    if (lba_to_chs(lba + d->first_sector, &chs, &total))
        return -1; /* Invalid LBA address */

    AH(regs) = operation;   /* Can be STD_READ, STD_WRITE or STD_VERIFY */
    AL(regs) = num_sectors; /* Must be nonzero                          */
    CH(regs) = (BYTE) chs.c;
    CL(regs) = chs.s | ((chs.c & 0x300) >> 2);
    DH(regs) = chs.h;
    DL(regs) = d->bios_number;
    ES(regs) = buf_seg;
    BX(regs) = buf_off;

    fd32_int(0x13, regs);
    if (FLAGS(regs) & 0x1) return -1; //INT 13h error code in AH
    return 0;
}


/* Reads data from disk sectors into a user buffer, using the */
/* standard BIOS service INT 13h/AH=02h.                      */
/* Return zero on success, nonzero on failure.                */
int biosdisk_stdread(const Disk *d, DWORD start, DWORD count, void *buffer)
{
    int         res;
    LOWMEM_ADDR buf_sel;
    WORD        buf_segment, buf_offset;

    buf_sel = fd32_dosmem_get(count * d->block_size, &buf_segment, &buf_offset);
    res = std_disk_op(STD_READ, d, start, count, buf_segment, buf_offset);
    fd32_memcpy_from_lowmem(buffer, buf_sel, 0, count * d->block_size);
    fd32_dosmem_free(buf_sel, count * d->block_size);
    return res;
}


#ifdef BIOSDISK_WRITE
/* Writes data from a user buffer into disk sectors, using */
/* the standard BIOS service INT 13h/AH=03h.               */
/* Return zero on success, nonzero on failure.             */
int biosdisk_stdwrite(const Disk *d, DWORD start, DWORD count, const void *buffer)
{
    int         res;
    LOWMEM_ADDR buf_sel;
    WORD        buf_segment, buf_offset;

    buf_sel = fd32_dosmem_get(count * d->block_size, &buf_segment, &buf_offset);
    fd32_memcpy_to_lowmem(buf_sel, 0, (void *) buffer, count * d->block_size);
    res = std_disk_op(STD_WRITE, d, start, count, buf_segment, buf_offset);
    fd32_dosmem_free(buf_sel, count * d->block_size);
    return res;
}
#endif
