/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2002, Salvatore Isaja                               *
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
tStdOp;


/* A CHS triplet */
typedef struct { DWORD C, H, S; } tChs;


/* Convert LBA 32-bit absolute sector number in CHS coordinates. */
/* Return nonzero if LBA is too large for CHS rappresentation.   */
static inline int lba_to_chs(DWORD Lba, tChs *Chs,
                             DWORD NumC, DWORD NumH, DWORD NumS)
{
  DWORD Temp;
  if (Lba >= NumC * NumH * NumS) return -1;
  Chs->C   = Lba / (NumH * NumS);
  Temp     = Lba % (NumH * NumS);
  Chs->H   = Temp / NumS;
  Chs->S   = Temp % NumS + 1;
  return 0;
}


/* Wrapper function for standard INT 13h 'read', 'write' and 'verify' */
/* services. Returns zero on success, nonzero on failure.             */
static inline int std_disk_op(tStdOp Operation, tDisk *D, DWORD Lba,
                              BYTE NumSectors, WORD BufSeg, WORD BufOff)
{
  FD32_DECLARE_REGS(Regs);
  tChs Chs;

  if (Lba >= D->TotalBlocks) return -1; /* Invalid LBA address */
  if (lba_to_chs(Lba + D->FirstSector, &Chs, D->BiosC, D->BiosH, D->BiosS))
    return -1; /* Invalid LBA address */

  AH(Regs) = Operation;  /* Can be STD_READ, STD_WRITE or STD_VERIFY */
  AL(Regs) = NumSectors; /* Must be nonzero                          */
  CH(Regs) = (BYTE) Chs.C;
  CL(Regs) = Chs.S | ((Chs.C & 0x300) >> 2);
  DH(Regs) = Chs.H;
  DL(Regs) = D->BiosNumber;
  ES(Regs) = BufSeg;
  BX(Regs) = BufOff;

  fd32_int(0x13, Regs);
  if (FLAGS(Regs) & 0x1) return -1; //INT 13h error code in AH
  return 0;
}


/* Reads data from disk sectors into a user buffer, using the */
/* standard BIOS service INT 13h/AH=02h.                      */
/* Return zero on success, nonzero on failure.                */
int biosdisk_stdread(tDisk *D, DWORD Start, DWORD Size, void *Buffer)
{
  int         Res;
  LOWMEM_ADDR BufSel;
  WORD        BufSegment, BufOffset;

  BufSel = fd32_dosmem_get(Size * D->BlockSize, &BufSegment, &BufOffset);
  Res = std_disk_op(STD_READ, D, Start, Size, BufSegment, BufOffset);
  fd32_memcpy_from_lowmem(Buffer, BufSel, 0, Size * D->BlockSize);
  fd32_dosmem_free(BufSel, Size * D->BlockSize);
  return Res;
}


#ifdef BIOSDISK_WRITE
/* Writes data from a user buffer into disk sectors, using */
/* the standard BIOS service INT 13h/AH=03h.               */
/* Return zero on success, nonzero on failure.             */
int biosdisk_stdwrite(tDisk *D, DWORD Start, DWORD Size, void *Buffer)
{
  int         Res;
  LOWMEM_ADDR BufSel;
  WORD        BufSegment, BufOffset;

  BufSel = fd32_dosmem_get(Size * D->BlockSize, &BufSegment, &BufOffset);
  fd32_memcpy_to_lowmem(BufSel, 0, Buffer, Size * D->BlockSize);
  Res = std_disk_op(STD_WRITE, D, Start, Size, BufSegment, BufOffset);
  fd32_dosmem_free(BufSel, Size * D->BlockSize);
  return Res;
}
#endif
