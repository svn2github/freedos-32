/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2002, Salvatore Isaja                               *
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
tExtOp;


/* Address packet to specify disk positions in extended INT13h services */
typedef struct
{
  BYTE   Size;      /* Size of this structure: 10h or 18h                  */
  BYTE   Reserved;  /* 0                                                   */
  WORD   NumBlocks; /* Max 007Fh for Phoenix EDD                           */
  DWORD  BufAddr;   /* Seg:off pointer to transfer buffer                  */
  QWORD  Start;     /* Starting absolute block number                      */
  QWORD  FlatAddr;  /* (EDD-3.0, optional) 64-bit flat address of transfer */
                    /* buffer; used if DWORD at 04h is FFFFh:FFFFh         */
}
__attribute__ ((packed)) tAddressPacket;


/* Wrapper function for extended INT13h 'read', 'write' and 'verify' */
/* services. Returns zero on success, nonzero on failure.            */
static inline int ext_disk_op(tExtOp Operation, BYTE Drive, DWORD Lba,
                              BYTE NumSectors, WORD BufSeg, WORD BufOff)
{
  FD32_DECLARE_REGS(Regs);
  tAddressPacket Ap;
  LOWMEM_ADDR    BufSelector;
  WORD           BufSegment, BufOffset;

  BufSelector = fd32_dosmem_get(sizeof(tAddressPacket),
                                &BufSegment, &BufOffset);
  Ap.Size      = 0x10;
  Ap.Reserved  = 0x00;
  Ap.NumBlocks = NumSectors;
  Ap.BufAddr   = (BufSeg << 16) + BufOff;
  Ap.Start     = Lba;
  Ap.FlatAddr  = 0;
  fd32_memcpy_to_lowmem(BufSelector, 0, &Ap, sizeof(tAddressPacket));

  AH(Regs) = Operation; /* Can be EXT_READ, EXT_WRITE or EXT_VERIFY */
  DL(Regs) = Drive;
  DS(Regs) = BufSegment;
  SI(Regs) = BufOffset;
  fd32_int(0x13, Regs);

  fd32_dosmem_free(BufSelector, sizeof(tAddressPacket));

  if (FLAGS(Regs) & 0x1) return -1; //INT 13h error code in AH
  return 0;
}


/* Reads data from disk sectors into a user buffer, using the extended */
/* BIOS service INT 13h/AH=42h.                                        */
/* Returns zero on success, nonzero on failure.                        */
int biosdisk_extread(tDisk *D, DWORD Start, DWORD Size, void *Buffer)
{
  int         Res;
  LOWMEM_ADDR BufSelector;
  WORD        BufSegment, BufOffset;

  if (Start >= D->TotalBlocks) return -1;
  BufSelector = fd32_dosmem_get(Size * D->BlockSize, &BufSegment, &BufOffset);
  Res = ext_disk_op(EXT_READ, D->BiosNumber, Start + D->FirstSector, Size,
                    BufSegment, BufOffset);
  fd32_memcpy_from_lowmem(Buffer, BufSelector, 0, Size * D->BlockSize);
  fd32_dosmem_free(BufSelector, Size * D->BlockSize);
  return Res;
}


#ifdef BIOSDISK_WRITE
/* Writes data from a user buffer into disk sectors, using the extended */
/* BIOS service INT 13h/AH=43h.                                         */
/* Returns zero on success, nonzero on failure.                         */
int biosdisk_extwrite(tDisk *D, DWORD Start, DWORD Size, void *Buffer)
{
  int         Res;
  LOWMEM_ADDR BufSelector;
  WORD        BufSegment, BufOffset;

  if (Start >= D->TotalBlocks) return -1;
  BufSelector = fd32_dosmem_get(Size * D->BlockSize, &BufSegment, &BufOffset);
  fd32_memcpy_to_lowmem(BufSelector, 0, Buffer, Size * D->BlockSize);
  Res = ext_disk_op(EXT_WRITE, D->BiosNumber, Start + D->FirstSector, Size,
                    BufSegment, BufOffset);
  fd32_dosmem_free(BufSelector, Size * D->BlockSize);
  return Res;
}
#endif
