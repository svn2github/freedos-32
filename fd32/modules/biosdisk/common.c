/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2002, Salvatore Isaja                               *
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


/* Reads sectors from the disk using the appropriate read function */
int biosdisk_read(tDisk *D, DWORD Start, DWORD Size, void *Buffer)
{
  if (D->PrivFlags & EXTAVAIL) return biosdisk_extread(D, Start, Size, Buffer);
                          else return biosdisk_stdread(D, Start, Size, Buffer);
}


#ifdef BIOSDISK_WRITE
/* Writes sectors to the disk using the appropriate write function */
int biosdisk_write(tDisk *D, DWORD Start, DWORD Size, void *Buffer)
{
  if (D->PrivFlags & EXTAVAIL) return biosdisk_extwrite(D, Start, Size, Buffer);
                          else return biosdisk_stdwrite(D, Start, Size, Buffer);
}
#endif


int biosdisk_devopen(tDisk *D)
{
  return ++D->OpenCount;
}


int biosdisk_devclose(tDisk *D)
{
  if (--D->OpenCount == 0) /* Flush buffered data */;
  return D->OpenCount;
}


/* Wrapper function for standard INT 13h 'detect disk change' service. */
/* Returns 0 if disk has not been changed, a positive number if disk   */
/* has been changed or a negative number on error.                     */
int biosdisk_mediachange(tDisk *D)
{
  FD32_DECLARE_REGS(Regs);

  AH(Regs) = 0x16;   /* Floppy disk - Detect disk change */
  DL(Regs) = D->BiosNumber;
  SI(Regs) = 0x0000; /* to avoid crash on AT&T 6300 */
  fd32_int(0x13, Regs);
  if (!(FLAGS(Regs) & 0x1)) return 0; /* Carry clear is disk not changed */
  if (AH(Regs) == 0x06) return 1; /* Disk changed */
  return -1; /* If AH is not 06h an error occurred */
}
