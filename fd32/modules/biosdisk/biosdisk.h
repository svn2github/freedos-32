/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2002, Salvatore Isaja                               *
 *                                                                        *
 * This is "biosdisk.h" - Defines and declarations                        *
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

#ifndef __FD32_BIOSDISK_H
#define __FD32_BIOSDISK_H

#include <dr-env.h>
#include <devices.h>

/* Use the following defines to add features to the BIOSDisk driver */
#define BIOSDISK_WRITE   /* Define this to enable write support           */
#define BIOSDISK_FD32DEV /* Define this to register FD32 devices          */
#define BIOSDISK_SHOWMSG /* Define this to show messages during detection */

/* Mnemonics for the private flags of the tDisk structure */
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
typedef struct
{
  int   OpenCount;
  DWORD FirstSector;
  DWORD BiosNumber;
  DWORD PrivFlags;
  DWORD BiosC, BiosH, BiosS;
  DWORD PhysC, PhysH, PhysS;
  DWORD BlockSize;   /* As defined in FD32_BLOCKINFO */
  DWORD TotalBlocks; /* As defined in FD32_BLOCKINFO */
  DWORD Type;        /* As defined in FD32_BLOCKINFO */
}
tDisk;

/* Standard BIOS disk functions */
int biosdisk_stdread (tDisk *D, DWORD Start, DWORD Size, void *Buffer);
int biosdisk_stdwrite(tDisk *D, DWORD Start, DWORD Size, void *Buffer);

/* IBM/MS Extended BIOS disk functions */
int biosdisk_extread (tDisk *D, DWORD Start, DWORD Size, void *Buffer);
int biosdisk_extwrite(tDisk *D, DWORD Start, DWORD Size, void *Buffer);

/* Operations common to Standard and IBM/MS Extended BIOSes */
int biosdisk_read       (tDisk *D, DWORD Start, DWORD Size, void *Buffer);
int biosdisk_write      (tDisk *D, DWORD Start, DWORD Size, void *Buffer);
int biosdisk_devopen    (tDisk *D);
int biosdisk_devclose   (tDisk *D);
int biosdisk_mediachange(tDisk *D);

/* Initialization functions */
int biosdisk_detect  (void);
int biosdisk_scanpart(tDisk *D, char *DevName);

/* Driver request function */
fd32_request_t biosdisk_request;

#endif /* #ifndef __FD32_BIOSDISK_H */

