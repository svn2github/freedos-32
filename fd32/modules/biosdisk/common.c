/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "common.c" - Operations common to Standard and IBM/MS Extended *
 *                      BIOSes and to floppy and hard disk devices        *
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

#include "biosdisk.h"

#define NUMRETRIES 5 /* Number of times to retry on error */


/* Resets the disk system, as required between access retries */
static int biosdisk_reset(const Disk *d)
{
    FD32_DECLARE_REGS(regs);

    AH(regs) = 0x00; /* Reset disk system */
    DL(regs) = d->bios_number;
    fd32_int(0x13, regs);
    if (FLAGS(regs) & 0x1) return -1; /* Error code in AH */
    return 0;
}


/* Reads sectors from the disk using the appropriate read function */
int biosdisk_read(const Disk *d, DWORD start, DWORD count, void *buffer)
{
    int      res;
    unsigned k;
    for (k = 0; k < NUMRETRIES; k++)
    {
        if (d->priv_flags & EXTAVAIL)
            res = biosdisk_extread(d, start, count, buffer);
          else
            res = biosdisk_stdread(d, start, count, buffer);
        if (res == 0) return 0;
        biosdisk_reset(d);
    }
    return res;
}


#ifdef BIOSDISK_WRITE
/* Writes sectors to the disk using the appropriate write function */
int biosdisk_write(const Disk *d, DWORD start, DWORD count, const void *buffer)
{
    int      res;
    unsigned k;
    for (k = 0; k < NUMRETRIES; k++)
    {
        if (d->priv_flags & EXTAVAIL)
            res = biosdisk_extwrite(d, start, count, buffer);
          else
            res = biosdisk_stdwrite(d, start, count, buffer);
        if (res == 0) return 0;
        biosdisk_reset(d);
    }
    return res;
}
#endif


int biosdisk_devopen(Disk *d)
{
    return ++d->open_count;
}


int biosdisk_devclose(Disk *d)
{
    if (--d->open_count == 0) /* Flush buffered data */;
    return d->open_count;
}


/* Wrapper function for standard INT 13h 'detect disk change' service. */
/* Returns 0 if disk has not been changed, a positive number if disk   */
/* has been changed or a negative number on error.                     */
int biosdisk_mediachange(const Disk *d)
{
    FD32_DECLARE_REGS(regs);

    AH(regs) = 0x16;   /* Floppy disk - Detect disk change */
    DL(regs) = d->bios_number;
    SI(regs) = 0x0000; /* to avoid crash on AT&T 6300 */
    fd32_int(0x13, regs);
    if (!(FLAGS(regs) & 0x1)) return 0; /* Carry clear is disk not changed */
    if (AH(regs) == 0x06) return 1; /* Disk changed */
    return -1; /* If AH is not 06h an error occurred */
}
